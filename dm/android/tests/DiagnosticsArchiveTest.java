package com.xiaoboluo.d2r;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import java.util.zip.*;

public final class DiagnosticsArchiveTest {
    private static Map<String, byte[]> unzip(byte[] bytes) throws IOException {
        Map<String, byte[]> entries = new LinkedHashMap<>();
        try (ZipInputStream in = new ZipInputStream(new ByteArrayInputStream(bytes))) {
            for (ZipEntry e; (e = in.getNextEntry()) != null;) entries.put(e.getName(), in.readAllBytes());
        }
        return entries;
    }
    private static void expectFailure(File files, File runtime, OutputStream out) throws Exception {
        try { DiagnosticsArchive.write(files, runtime, "test=yes\n", out); }
        catch (IOException expected) { return; }
        throw new AssertionError("output failure was reported as success");
    }
    public static void main(String[] args) throws Exception {
        File root = Files.createTempDirectory("dm-diagnostics").toFile();
        File files = new File(root, "files"), runtime = new File(files, "dm");
        assert runtime.mkdirs();
        Files.writeString(new File(runtime, "boot.log").toPath(), "boot: path=" + files + "/save\n[dm] CRASH sig=11\n");
        Files.writeString(new File(runtime, "main.patched.bin").toPath(), "private binary");
        Files.writeString(new File(files, "perf.txt").toPath(), "frame\tgpu\tscale\n");
        File saves = new File(files, "save"); assert saves.mkdir();
        Files.writeString(new File(saves, "hero.d2s").toPath(), "private save");
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        DiagnosticsArchive.write(files, runtime, "build=test\n", output);
        Map<String, byte[]> entries = unzip(output.toByteArray());
        assert entries.keySet().equals(Set.of("metadata.txt", "boot.log", "perf.txt")) : entries.keySet();
        String metadata = new String(entries.get("metadata.txt"), StandardCharsets.UTF_8);
        assert metadata.contains("perf.old.txt=missing");
        assert metadata.contains("build=test");
        assert !new String(entries.get("boot.log"), StandardCharsets.UTF_8).contains(files.toString());
        assert new String(entries.get("boot.log"), StandardCharsets.UTF_8).contains("CRASH sig=11");
        output.reset();
        DiagnosticsArchive.write(files, runtime, "build=test\n", "NVN slow-pso frame=74706 key=123\nprivate_path=" + runtime, output);
        entries = unzip(output.toByteArray());
        assert entries.containsKey("pipeline.log");
        String pipeline = new String(entries.get("pipeline.log"), StandardCharsets.UTF_8);
        assert pipeline.contains("frame=74706") && !pipeline.contains(runtime.toString());
        assert new String(entries.get("metadata.txt"), StandardCharsets.UTF_8).contains("pipeline.log=process-memory snapshot");
        output.reset(); DiagnosticsArchive.write(files, runtime, "test=yes", "x".repeat(DiagnosticsArchive.LOG_LIMIT * 2), output);
        assert unzip(output.toByteArray()).get("pipeline.log").length <= DiagnosticsArchive.LOG_LIMIT;
        byte[] huge = new byte[DiagnosticsArchive.LOG_LIMIT * 2]; Arrays.fill(huge, (byte)'x');
        byte[] marker = "\nLAST-FRAME\n".getBytes(StandardCharsets.UTF_8);
        System.arraycopy(marker, 0, huge, huge.length - marker.length, marker.length);
        Files.write(new File(files, "perf.txt").toPath(), huge);
        output.reset(); DiagnosticsArchive.write(files, runtime, "build=test\n", output);
        entries = unzip(output.toByteArray());
        assert entries.get("perf.txt").length <= DiagnosticsArchive.LOG_LIMIT;
        assert new String(entries.get("perf.txt"), StandardCharsets.UTF_8).endsWith("LAST-FRAME\n");
        assert new String(entries.get("metadata.txt"), StandardCharsets.UTF_8).contains("perf.txt=tail");
        byte[] invalid = new byte[DiagnosticsArchive.LOG_LIMIT]; Arrays.fill(invalid, (byte)0xff);
        Files.write(new File(files, "perf.old.txt").toPath(), invalid);
        output.reset(); DiagnosticsArchive.write(files, runtime, "build=test\n", output);
        assert unzip(output.toByteArray()).get("perf.old.txt").length <= DiagnosticsArchive.LOG_LIMIT : "invalid UTF-8 expanded past the byte cap";
        Files.delete(new File(files, "perf.old.txt").toPath());
        File secret = new File(root, "secret"); Files.writeString(secret.toPath(), "DO NOT EXPORT");
        try {
            File alias = new File(root, "parent-alias");
            Files.createSymbolicLink(alias.toPath(), root.toPath());
            File aliasedFiles = new File(alias, "files"), aliasedRuntime = new File(aliasedFiles, "dm");
            output.reset(); DiagnosticsArchive.write(aliasedFiles, aliasedRuntime, "build=test\n", output);
            assert unzip(output.toByteArray()).containsKey("boot.log") : "standard parent alias lost diagnostic logs";
            Files.createSymbolicLink(new File(files, "perf.old.txt").toPath(), secret.toPath());
            output.reset(); DiagnosticsArchive.write(files, runtime, "build=test\n", output);
            assert !unzip(output.toByteArray()).containsKey("perf.old.txt");
        } catch (UnsupportedOperationException | java.nio.file.FileSystemException e) {
            System.out.println("SKIP symlink creation unavailable: " + e.getClass().getSimpleName());
        }
        expectFailure(files, runtime, new OutputStream() { public void write(int b) throws IOException { throw new IOException("disk full"); } });
        expectFailure(files, runtime, new ByteArrayOutputStream() { public void close() throws IOException { throw new IOException("provider close failed"); } });
        System.out.println("PASS whitelist, bounded tail, privacy, missing logs, write and close failures");
    }
}
