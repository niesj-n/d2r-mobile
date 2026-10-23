package com.xiaoboluo.d2r;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.ByteBuffer;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.StandardOpenOption;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

/** Explicit local diagnostic files only; never enumerate the game/runtime directories. */
final class DiagnosticsArchive {
    static final int LOG_LIMIT = 256 * 1024;
    private static final int METADATA_LIMIT = 16 * 1024;

    private static byte[] tail(File file) throws IOException {
        try (SeekableByteChannel input = Files.newByteChannel(file.toPath(), StandardOpenOption.READ, LinkOption.NOFOLLOW_LINKS)) {
            long size = input.size();
            int count = (int)Math.min(size, LOG_LIMIT);
            byte[] data = new byte[count];
            input.position(size - count);
            ByteBuffer buffer = ByteBuffer.wrap(data);
            while (buffer.hasRemaining()) if (input.read(buffer) < 0) throw new EOFException("Log changed while reading");
            // A truncated UTF-8 prefix may split a character; discard its partial line.
            if (size > count) for (int i = 0; i < data.length - 1; ++i) if (data[i] == '\n') {
                return java.util.Arrays.copyOfRange(data, i + 1, data.length);
            }
            return data;
        }
    }

    private static byte[] scrub(byte[] data, File files, File runtime) {
        String text = new String(data, StandardCharsets.UTF_8);
        text = text.replace(runtime.getAbsolutePath(), "<runtime>")
                   .replace(files.getAbsolutePath(), "<files>");
        try { text = text.replace(runtime.toPath().toRealPath().toString(), "<runtime>")
                         .replace(files.toPath().toRealPath().toString(), "<files>"); }
        catch (IOException ignored) { /* The literal app paths above are still removed. */ }
        byte[] encoded = text.getBytes(StandardCharsets.UTF_8);
        if (encoded.length <= LOG_LIMIT) return encoded;
        int from = encoded.length - LOG_LIMIT;
        while (from < encoded.length && (encoded[from] & 0xc0) == 0x80) ++from;
        return java.util.Arrays.copyOfRange(encoded, from, encoded.length);
    }

    /** Owns and closes target. A failed flush/close must propagate to the UI. */
    static void write(File files, File runtime, String metadata, OutputStream target) throws IOException {
        if (target == null) throw new IOException("Cannot open selected destination");
        try (ZipOutputStream zip = new ZipOutputStream(target)) {
            Map<String, File> allowed = new LinkedHashMap<>();
            allowed.put("boot.log", new File(runtime, "boot.log"));
            allowed.put("perf.txt", new File(files, "perf.txt"));
            allowed.put("perf.old.txt", new File(files, "perf.old.txt"));
            Map<String, byte[]> content = new LinkedHashMap<>();
            StringBuilder header = new StringBuilder(metadata.substring(0, Math.min(metadata.length(), METADATA_LIMIT)));
            header.append("\nformat=1\nper_log_limit_bytes=").append(LOG_LIMIT)
                .append("\ncrash_source=boot.log stderr; last fatal lines may be lost on process exit\n");
            for (Map.Entry<String, File> entry : allowed.entrySet()) {
                File file = entry.getValue(); String status;
                if (!Files.exists(file.toPath(), LinkOption.NOFOLLOW_LINKS)) status = "missing";
                else if (!Files.isRegularFile(file.toPath(), LinkOption.NOFOLLOW_LINKS) ||
                         Files.isSymbolicLink(file.getParentFile().toPath()) ||
                         !file.toPath().toRealPath().equals(file.getParentFile().toPath().toRealPath().resolve(file.getName())))
                    status = "rejected non-regular or linked path";
                else {
                    try {
                        long length = file.length();
                        content.put(entry.getKey(), scrub(tail(file), files, runtime));
                        status = (length > LOG_LIMIT ? "tail" : "complete") + "; source_bytes=" + length;
                    } catch (IOException e) { status = "unreadable (" + e.getClass().getSimpleName() + ")"; }
                }
                header.append(entry.getKey()).append('=').append(status).append('\n');
            }
            zip.putNextEntry(new ZipEntry("metadata.txt"));
            zip.write(scrub(header.toString().getBytes(StandardCharsets.UTF_8), files, runtime)); zip.closeEntry();
            for (Map.Entry<String, byte[]> entry : content.entrySet()) {
                zip.putNextEntry(new ZipEntry(entry.getKey())); zip.write(entry.getValue()); zip.closeEntry();
            }
        }
    }
}
