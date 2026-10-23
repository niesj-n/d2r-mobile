"""Check the actual APK's native hooks, extraction mode and 16 KB ELF segments."""
import struct
import sys
import zipfile

apk = sys.argv[1]
expected = {"libdm.so", "libhook_impl.so", "libmain_hook.so"}
with zipfile.ZipFile(apk) as archive:
    libraries = [entry for entry in archive.infolist() if entry.filename.startswith("lib/arm64-v8a/") and entry.filename.endswith(".so")]
    assert {entry.filename.rsplit("/", 1)[-1] for entry in libraries} == expected
    for entry in libraries:
        assert entry.compress_type == zipfile.ZIP_DEFLATED, entry.filename
        data = archive.read(entry)
        assert data[:6] == b"\x7fELF\x02\x01", entry.filename
        program_offset = struct.unpack_from("<Q", data, 32)[0]
        size = struct.unpack_from("<H", data, 54)[0]
        count = struct.unpack_from("<H", data, 56)[0]
        loads = 0
        for index in range(count):
            off = program_offset + index * size
            if struct.unpack_from("<I", data, off)[0] != 1:
                continue
            loads += 1
            file_offset = struct.unpack_from("<Q", data, off + 8)[0]
            virtual_address = struct.unpack_from("<Q", data, off + 16)[0]
            alignment = struct.unpack_from("<Q", data, off + 48)[0]
            assert alignment >= 16384 and (file_offset - virtual_address) % alignment == 0, entry.filename
        assert loads > 0, entry.filename
print("PASS APK contains extracted AdrenoTools hooks and three 16 KB-aligned ARM64 ELF libraries")
