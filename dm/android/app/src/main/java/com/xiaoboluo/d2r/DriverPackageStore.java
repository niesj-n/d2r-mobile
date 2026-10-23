package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/** Private, immutable driver ZIPs. No document URI or raw path is saved as a choice. */
final class DriverPackageStore {
    private static final long ARCHIVE_LIMIT = 256L << 20;
    private static final long EXPANDED_LIMIT = 512L << 20;
    private static final long LIBRARY_LIMIT = 200L << 20;
    private static final int ENTRY_LIMIT = 32;
    private static final Pattern DIGEST = Pattern.compile("[0-9a-f]{64}");
    private static final Pattern BASENAME = Pattern.compile("[A-Za-z0-9][A-Za-z0-9._-]{0,79}");
    private static final Pattern META_NAME = Pattern.compile("\"libraryName\"\\s*:\\s*\"([A-Za-z0-9._-]+)\"");

    static final class DriverInfo {
        final String digest;
        final String libraryName;
        final File directory;
        private DriverInfo(String digest, String libraryName, File directory) {
            this.digest = digest; this.libraryName = libraryName; this.directory = directory;
        }
    }

    private DriverPackageStore() {}
    private static File drivers(File filesDir) { return new File(filesDir, "drivers"); }
    private static File selectionFile(File filesDir) { return new File(drivers(filesDir), "selected.txt"); }
    static File attemptFile(File filesDir) { return new File(drivers(filesDir), "attempt.pending"); }
    static void cleanupIncomplete(File filesDir) throws IOException {
        File[] leftovers = drivers(filesDir).listFiles((dir, name) ->
            name.matches("\\.(archive|import)-[0-9a-fA-F-]{36}"));
        if (leftovers != null) for (File leftover : leftovers) deleteTree(leftover.toPath());
    }

    private static String hex(byte[] bytes) {
        char[] out = new char[bytes.length * 2];
        final char[] digits = "0123456789abcdef".toCharArray();
        for (int i = 0; i < bytes.length; i++) { out[2*i] = digits[(bytes[i] >> 4) & 15]; out[2*i+1] = digits[bytes[i] & 15]; }
        return new String(out);
    }
    private static MessageDigest sha256() {
        try { return MessageDigest.getInstance("SHA-256"); }
        catch (NoSuchAlgorithmException impossible) { throw new AssertionError(impossible); }
    }
    private static void ensureRoot(File filesDir) throws IOException {
        File root = drivers(filesDir);
        if (!root.isDirectory() && !root.mkdirs()) throw new IOException("Cannot create private driver directory");
    }
    private static void deleteTree(Path path) throws IOException {
        if (!Files.exists(path)) return;
        Files.walkFileTree(path, new SimpleFileVisitor<>() {
            @Override public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException { Files.delete(file); return FileVisitResult.CONTINUE; }
            @Override public FileVisitResult postVisitDirectory(Path dir, IOException error) throws IOException {
                if (error != null) throw error;
                Files.delete(dir); return FileVisitResult.CONTINUE;
            }
        });
    }
    private static boolean safeName(String name) {
        return BASENAME.matcher(name).matches() && !name.contains("..");
    }
    private static boolean mainLibraryName(String name) {
        if (!safeName(name) || !name.endsWith(".so")) return false;
        return (name.startsWith("vulkan.") && name.length() > 10) ||
               (name.startsWith("libvulkan_") && name.length() > 13);
    }
    private static String entryName(String raw, String commonFolder) throws IOException {
        if (raw.indexOf('\\') >= 0 || raw.indexOf(':') >= 0 || raw.startsWith("/") || raw.indexOf('\0') >= 0)
            throw new IOException("Unsafe ZIP path");
        String name = raw.endsWith("/") ? raw.substring(0, raw.length() - 1) : raw;
        String[] parts = name.split("/", -1);
        if (parts.length > 2 || parts.length == 0 || !safeName(parts[0])) throw new IOException("Unsafe ZIP path");
        if (parts.length == 1) {
            if (commonFolder != null) throw new IOException("Mixed ZIP roots");
            return parts[0];
        }
        if (!parts[0].equals(commonFolder) || !safeName(parts[1])) throw new IOException("Mixed or unsafe ZIP roots");
        return parts[1];
    }
    private static void verifyElf(File file) throws IOException {
        byte[] h = new byte[64];
        try (FileInputStream in = new FileInputStream(file)) {
            if (in.read(h) != h.length) throw new IOException("Short ELF library");
        }
        if (h[0] != 0x7f || h[1] != 'E' || h[2] != 'L' || h[3] != 'F' || h[4] != 2 || h[5] != 1)
            throw new IOException("Not a 64-bit little-endian ELF library");
        ByteBuffer b = ByteBuffer.wrap(h).order(ByteOrder.LITTLE_ENDIAN);
        if (Short.toUnsignedInt(b.getShort(16)) != 2 || Short.toUnsignedInt(b.getShort(18)) != 183)
            throw new IOException("Driver must be an ARM64 shared library");
    }
    private static String selectMain(List<String> names, String metadata) throws IOException {
        Matcher matcher = META_NAME.matcher(metadata);
        boolean namedLibrary = matcher.find();
        if (metadata.contains("\"libraryName\"") && !namedLibrary) throw new IOException("Invalid libraryName metadata");
        if (namedLibrary) {
            String named = matcher.group(1);
            if (!mainLibraryName(named) || !names.contains(named)) throw new IOException("Missing named Vulkan library");
            return named;
        }
        String main = null;
        for (String name : names) if (mainLibraryName(name)) {
            if (main != null) throw new IOException("Ambiguous Vulkan libraries");
            main = name;
        }
        if (main == null) throw new IOException("No Vulkan driver library");
        return main;
    }
    private static void writeSynced(File target, byte[] data) throws IOException {
        try (FileOutputStream out = new FileOutputStream(target)) { out.write(data); out.getFD().sync(); }
    }
    private static String readText(File file, java.nio.charset.Charset charset) throws IOException {
        return new String(Files.readAllBytes(file.toPath()), charset);
    }
    private static void atomicText(File target, String text) throws IOException {
        File temp = new File(target.getParentFile(), "." + target.getName() + "." + UUID.randomUUID());
        try {
            writeSynced(temp, text.getBytes(StandardCharsets.UTF_8));
            Files.move(temp.toPath(), target.toPath(), StandardCopyOption.ATOMIC_MOVE, StandardCopyOption.REPLACE_EXISTING);
        } finally { Files.deleteIfExists(temp.toPath()); }
    }

    static DriverInfo importPackage(File filesDir, InputStream input) throws IOException {
        ensureRoot(filesDir);
        File root = drivers(filesDir), archive = new File(root, ".archive-" + UUID.randomUUID());
        File stage = new File(root, ".import-" + UUID.randomUUID());
        MessageDigest hash = sha256();
        try {
            try (FileOutputStream out = new FileOutputStream(archive)) {
                byte[] bytes = new byte[65536]; long total = 0; int n;
                while ((n = input.read(bytes)) != -1) {
                    total += n;
                    if (total > ARCHIVE_LIMIT) throw new IOException("Driver ZIP too large");
                    hash.update(bytes, 0, n); out.write(bytes, 0, n);
                }
                out.getFD().sync();
            }
            String digest = hex(hash.digest());
            if (!stage.mkdir()) throw new IOException("Cannot create import staging directory");
            List<String> names = new ArrayList<>(); Set<String> unique = new HashSet<>();
            String metadata = ""; long expanded = 0; int entries = 0;
            try (ZipFile zip = new ZipFile(archive)) {
                List<ZipEntry> files = new ArrayList<>();
                Enumeration<? extends ZipEntry> e = zip.entries();
                String folder = null; boolean rootFiles = false;
                while (e.hasMoreElements()) {
                    ZipEntry z = e.nextElement();
                    if (++entries > ENTRY_LIMIT) throw new IOException("Too many driver ZIP entries");
                    String raw = z.getName();
                    if (raw.indexOf('\\') >= 0 || raw.indexOf(':') >= 0 || raw.startsWith("/") || raw.contains(".."))
                        throw new IOException("Unsafe ZIP path");
                    if (z.isDirectory()) continue;
                    int slash = raw.indexOf('/');
                    if (slash >= 0) {
                        if (folder == null) folder = raw.substring(0, slash);
                        if (rootFiles || !folder.equals(raw.substring(0, slash))) throw new IOException("Mixed ZIP roots");
                    } else { if (folder != null) throw new IOException("Mixed ZIP roots"); rootFiles = true; }
                    files.add(z);
                }
                for (ZipEntry z : files) {
                    String name = entryName(z.getName(), folder);
                    if (!unique.add(name)) throw new IOException("Duplicate ZIP entry");
                    if (!name.endsWith(".so") && !name.equals("meta.json")) continue;
                    long limit = name.equals("meta.json") ? 65536 : LIBRARY_LIMIT;
                    if (z.getSize() > limit) throw new IOException("Driver entry too large");
                    File output = new File(stage, name);
                    long count = 0;
                    try (InputStream in = zip.getInputStream(z); FileOutputStream out = new FileOutputStream(output)) {
                        byte[] bytes = new byte[65536]; int n;
                        while ((n = in.read(bytes)) != -1) {
                            count += n; expanded += n;
                            if (count > limit || expanded > EXPANDED_LIMIT) throw new IOException("Driver ZIP expands too far");
                            out.write(bytes, 0, n);
                        }
                        out.getFD().sync();
                    }
                    if (name.equals("meta.json")) metadata = readText(output, StandardCharsets.UTF_8);
                    else { verifyElf(output); names.add(name); }
                }
            }
            String main = selectMain(names, metadata);
            writeSynced(new File(stage, "driver.txt"), (digest + "\n" + main + "\n").getBytes(StandardCharsets.US_ASCII));
            File finalDir = new File(root, digest);
            if (!finalDir.exists()) Files.move(stage.toPath(), finalDir.toPath(), StandardCopyOption.ATOMIC_MOVE);
            DriverInfo result = readInfo(filesDir, digest);
            if (result == null || !result.libraryName.equals(main)) throw new IOException("Driver package directory changed");
            return result;
        } finally {
            deleteTree(stage.toPath());
            Files.deleteIfExists(archive.toPath());
        }
    }
    private static DriverInfo readInfo(File filesDir, String digest) {
        if (digest == null || !DIGEST.matcher(digest).matches()) return null;
        File directory = new File(drivers(filesDir), digest), manifest = new File(directory, "driver.txt");
        try {
            String[] lines = readText(manifest, StandardCharsets.US_ASCII).split("\\R");
            if (lines.length < 2 || !digest.equals(lines[0]) || !mainLibraryName(lines[1])) return null;
            File library = new File(directory, lines[1]);
            if (!library.isFile()) return null;
            return new DriverInfo(digest, lines[1], directory);
        } catch (IOException e) { return null; }
    }
    static List<DriverInfo> list(File filesDir) {
        List<DriverInfo> found = new ArrayList<>();
        File[] dirs = drivers(filesDir).listFiles();
        if (dirs != null) for (File dir : dirs) {
            DriverInfo info = readInfo(filesDir, dir.getName());
            if (info != null) found.add(info);
        }
        found.sort((a, b) -> a.digest.compareTo(b.digest));
        return found;
    }
    static DriverInfo selected(File filesDir) {
        try {
            String digest = readText(selectionFile(filesDir), StandardCharsets.US_ASCII).trim();
            return readInfo(filesDir, digest);
        } catch (IOException e) { return null; }
    }
    static void select(File filesDir, DriverInfo info) throws IOException {
        ensureRoot(filesDir);
        if (info == null || readInfo(filesDir, info.digest) == null) throw new IOException("Unimported driver");
        atomicText(selectionFile(filesDir), info.digest + "\n");
    }
    static void clearSelection(File filesDir) throws IOException { Files.deleteIfExists(selectionFile(filesDir).toPath()); }
    static void beginAttempt(File filesDir, DriverInfo info) throws IOException {
        if (info == null || readInfo(filesDir, info.digest) == null) throw new IOException("Unimported driver");
        atomicText(attemptFile(filesDir), info.digest + "\n");
    }
    static void cancelAttempt(File filesDir) throws IOException { Files.deleteIfExists(attemptFile(filesDir).toPath()); }
    static boolean recoverFailedAttempt(File filesDir) throws IOException {
        if (!attemptFile(filesDir).isFile()) return false;
        clearSelection(filesDir);
        Files.deleteIfExists(attemptFile(filesDir).toPath());
        return true;
    }
}
