"""
Compiles the C sources in c/ (your original bst/heap/hash/queue/stack/fee/
parking/file logic, plus the thin webapi.c bridge) into a shared library
that app.py loads with ctypes.

Usage:
    python build.py

Requires a C compiler on PATH:
  - Linux/macOS: gcc or clang (Xcode command line tools on macOS)
  - Windows: gcc via MinGW-w64 (e.g. from MSYS2) -- the same kind of
    toolchain that would have produced parking.exe originally.
"""

import platform
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).parent
C_DIR = ROOT / "c"

SOURCES = [
    "bst.c", "fee.c", "file.c", "hash.c",
    "heap.c", "parking.c", "queue.c", "stack.c", "webapi.c",
]

PE_MACHINE_NAMES = {
    0x014C: "32-bit (i386)",
    0x8664: "64-bit (AMD64)",
    0xAA64: "64-bit (ARM64)",
}


def library_name():
    system = platform.system()
    if system == "Windows":
        return "parking.dll"
    if system == "Darwin":
        return "libparking.dylib"
    return "libparking.so"


def find_compiler():
    for candidate in ("gcc", "cc", "clang"):
        path = shutil.which(candidate)
        if path:
            return candidate
    return None


def python_bitness():
    return struct.calcsize("P") * 8  # 32 or 64


def pe_machine_type(dll_path):
    """Reads a Windows .dll's PE header to find what architecture it was
    built for, without needing any extra tools. Returns a machine-type
    code (see PE_MACHINE_NAMES) or None if the file isn't a valid PE."""
    try:
        with open(dll_path, "rb") as f:
            data = f.read(1024)
        if data[:2] != b"MZ":
            return None
        pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe_offset:pe_offset + 4] != b"PE\x00\x00":
            return None
        machine = struct.unpack_from("<H", data, pe_offset + 4)[0]
        return machine
    except Exception:
        return None


def check_windows_bitness(out_path, compiler):
    """After building on Windows, verify the DLL's architecture matches this
    Python's architecture, and fail with a clear explanation instead of
    letting ctypes raise a cryptic WinError 193 later."""
    py_bits = python_bitness()
    machine = pe_machine_type(out_path)

    if machine is None:
        print(f"\nWarning: couldn't read {out_path.name}'s PE header to verify its "
              "architecture. If 'python app.py' fails with WinError 193, that "
              "means a 32/64-bit mismatch - see below.")
        return

    dll_bits = 32 if machine == 0x014C else 64
    dll_desc = PE_MACHINE_NAMES.get(machine, hex(machine))

    print(f"Built {out_path.name}: {dll_desc}. This Python is {py_bits}-bit.")

    if dll_bits != py_bits:
        print(
            f"\n*** MISMATCH: {out_path.name} is {dll_bits}-bit but your Python "
            f"is {py_bits}-bit. ***\n"
            f"'{compiler}' on your PATH is the wrong bitness for this Python. "
            "ctypes will fail with 'WinError 193: %1 is not a valid Win32 "
            "application' if you try to load it.\n\n"
            "Fix: install a matching MinGW-w64 toolchain, e.g. via MSYS2 "
            "(https://www.msys2.org/):\n"
            "  1. Install MSYS2, then open the 'MSYS2 MinGW64' terminal "
            f"(NOT 'MSYS2 MinGW32') if your Python is 64-bit (most installs are).\n"
            "  2. Run:  pacman -S mingw-w64-x86_64-gcc\n"
            "  3. Add C:\\msys64\\mingw64\\bin to your PATH (restart your terminal "
            "afterwards), and make sure it comes BEFORE any other gcc on PATH.\n"
            "  4. Re-run:  python build.py\n"
        )
        sys.exit(1)


def main():
    compiler = find_compiler()
    if not compiler:
        print("No C compiler found on PATH (looked for gcc, cc, clang).")
        if platform.system() == "Windows":
            print("Install MinGW-w64 (e.g. via MSYS2: https://www.msys2.org/) "
                  "and make sure gcc.exe is on PATH, then re-run this script.")
        else:
            print("Install gcc or clang with your system package manager, "
                  "then re-run this script.")
        sys.exit(1)

    out_name = library_name()
    out_path = ROOT / out_name

    cmd = [compiler, "-shared", "-fPIC", "-O2", "-o", str(out_path)]
    cmd += [str(C_DIR / s) for s in SOURCES]

    print("Building", out_name, "from", ", ".join(SOURCES))
    print(" ".join(cmd))

    result = subprocess.run(cmd)
    if result.returncode != 0:
        print("\nBuild failed - see the compiler output above.")
        sys.exit(result.returncode)

    if platform.system() == "Windows":
        check_windows_bitness(out_path, compiler)

    print(f"\nBuilt {out_path}. You can now run: python app.py")


if __name__ == "__main__":
    main()
