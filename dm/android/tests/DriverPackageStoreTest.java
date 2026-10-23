package com.xiaoboluo.d2r;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public final class DriverPackageStoreTest {
    private static byte[] elf(int machine, int type) {
        byte[] data = new byte[128];
        data[0] = 0x7f; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
        data[4] = 2; data[5] = 1; data[6] = 1;
        ByteBuffer b = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN);
        b.putShort(16, (short)type); b.putShort(18, (short)machine);
        b.putLong(32, 64); b.putShort(54, (short)56); b.putShort(56, (short)1);
        b.putInt(64, 1); b.putLong(64 + 48, 16384);
        return data;
    }
    private static byte[] elf(int machine) { return elf(machine, 3); }
    private static byte[] zip(Map<String, byte[]> files) throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        try (ZipOutputStream out = new ZipOutputStream(bytes)) {
            for (var entry : files.entrySet()) {
                out.putNextEntry(new ZipEntry(entry.getKey()));
                out.write(entry.getValue()); out.closeEntry();
            }
        }
        return bytes.toByteArray();
    }
    private static void reject(File root, Map<String, byte[]> files) throws IOException {
        try { DriverPackageStore.importPackage(root, new ByteArrayInputStream(zip(files))); }
        catch (IOException expected) { return; }
        throw new AssertionError("unsafe package accepted");
    }
    public static void main(String[] args) throws Exception {
        assert args.length == 1 : "pass a real NDK-built ARM64 .so";
        File root = Files.createTempDirectory("dm-driver-store").toFile();
        assert DriverPackageStore.selected(root) == null;
        byte[] actualNdklib = Files.readAllBytes(new File(args[0]).toPath());
        DriverPackageStore.DriverInfo real = DriverPackageStore.importPackage(root,
            new ByteArrayInputStream(zip(Map.of("libvulkan_freedreno.so", actualNdklib))));
        assert real.libraryName.equals("libvulkan_freedreno.so");
        var basic = Map.of("vulkan.adreno.so", elf(183));
        DriverPackageStore.DriverInfo first = DriverPackageStore.importPackage(root, new ByteArrayInputStream(zip(basic)));
        assert first.digest.length() == 64 && first.libraryName.equals("vulkan.adreno.so");
        assert new File(first.directory, first.libraryName).isFile();
        DriverPackageStore.select(root, first);
        assert DriverPackageStore.selected(root).digest.equals(first.digest);
        DriverPackageStore.beginAttempt(root, first);
        DriverPackageStore.cancelAttempt(root);  /* nativeStart rejected resources before opening Vulkan */
        assert !DriverPackageStore.attemptFile(root).exists() && DriverPackageStore.selected(root) != null;
        DriverPackageStore.beginAttempt(root, first);
        assert DriverPackageStore.recoverFailedAttempt(root);
        assert DriverPackageStore.selected(root) == null;

        Map<String, byte[]> mesa = new HashMap<>();
        mesa.put("Mesa/libvulkan_freedreno.so", elf(183));
        mesa.put("Mesa/meta.json", "{\"libraryName\":\"libvulkan_freedreno.so\",\"name\":\"Turnip\"}".getBytes());
        DriverPackageStore.DriverInfo turnip = DriverPackageStore.importPackage(root, new ByteArrayInputStream(zip(mesa)));
        assert turnip.libraryName.equals("libvulkan_freedreno.so");
        DriverPackageStore.select(root, turnip);
        assert DriverPackageStore.selected(root).digest.equals(turnip.digest);
        assert DriverPackageStore.list(root).size() == 3;
        File incomplete = new File(root, "drivers/.import-00000000-0000-0000-0000-000000000001");
        assert incomplete.mkdir();
        Files.write(new File(incomplete, "vulkan.adreno.so").toPath(), elf(183));
        DriverPackageStore.cleanupIncomplete(root);
        assert !incomplete.exists() && DriverPackageStore.selected(root).digest.equals(turnip.digest);

        reject(root, Map.of("../vulkan.adreno.so", elf(183)));
        reject(root, Map.of("/vulkan.adreno.so", elf(183)));
        reject(root, Map.of("vulkan.adreno.so", elf(62)));
        reject(root, Map.of("vulkan.adreno.so", elf(183, 2)));  /* ET_EXEC is not a shared object */
        reject(root, Map.of("note.txt", new byte[]{1}));
        reject(root, Map.of("vulkan.adreno.so", elf(183), "other/vulkan.adreno.so", elf(183)));
        assert DriverPackageStore.selected(root).digest.equals(turnip.digest);
        assert DriverPackageStore.list(root).size() == 3;
        System.out.println("PASS real NDK ARM64 ET_DYN import, ET_EXEC rejection, Mesa metadata, selection and crash recovery");
    }
}
