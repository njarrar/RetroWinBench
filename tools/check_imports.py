#!/usr/bin/env python3
"""Fail the build if winbench.exe imports anything Windows 98 lacks.

Every function below exists in the Windows 98 (first edition) system DLLs.
Newer calls (EnumDisplayDevicesA, GetDiskFreeSpaceExA, OpenGL) are looked up
at run time with GetProcAddress instead, so they do not appear here.
"""
import re, subprocess, sys

WIN98 = {
    "KERNEL32.dll": """CloseHandle CreateFileA DeleteFileA ExitProcess FlushFileBuffers GetCommandLineA
        GetCurrentThread GetDiskFreeSpaceA GetFileAttributesA GetModuleFileNameA GetModuleHandleA
        GetProcAddress GetSystemDirectoryA GetSystemInfo GetTempPathA GetThreadPriority GetTickCount
        GlobalMemoryStatus LoadLibraryA FreeLibrary QueryPerformanceCounter QueryPerformanceFrequency
        ReadFile SetFilePointer SetThreadPriority VirtualAlloc VirtualFree WriteFile GetVersionExA""",
    "USER32.dll": """AdjustWindowRect CreateWindowExA DefWindowProcA DestroyWindow DispatchMessageA
        EnableWindow GetDC GetMessageA LoadCursorA LoadIconA MoveWindow PeekMessageA PostQuitMessage
        RegisterClassA ReleaseDC SendMessageA SetWindowTextA ShowWindow TranslateMessage UpdateWindow""",
    "GDI32.dll": "CreateFontA GetStockObject",
    "ADVAPI32.dll": "RegCloseKey RegOpenKeyExA RegQueryValueExA",
}
ALLOWED = {dll.lower(): set(names.split()) for dll, names in WIN98.items()}

exe = sys.argv[1]
dump = subprocess.run(["i686-w64-mingw32-objdump", "-p", exe], capture_output=True, text=True, check=True).stdout

bad, dll, count = [], None, 0
for line in dump.splitlines():
    m = re.match(r"\s*DLL Name: (\S+)", line)
    if m:
        dll = m.group(1).lower()
        if dll not in ALLOWED:
            bad.append(f"DLL {m.group(1)}")
        continue
    m = re.match(r"\s+[0-9a-f]+\s+([0-9a-f]+\s+)?(\w+)$", line)
    if dll and m and m.group(2) not in ("Hint/Ord", "Name") and not line.strip().startswith("vma"):
        name = m.group(2)
        count += 1
        if name not in ALLOWED.get(dll, set()):
            bad.append(f"{dll}!{name}")
    if line.startswith("The Export Tables") or line.startswith("PE File Base Relocations"):
        dll = None

sub = re.search(r"MajorSubsystemVersion\s+(\d+)", dump).group(1)
if sub != "4":
    bad.append(f"subsystem version {sub} (Windows 98 needs 4)")
if bad:
    print("NOT Windows 98 safe:\n  " + "\n  ".join(bad))
    sys.exit(1)
print(f"imports OK for Windows 98: {count} functions, subsystem 4.0")
