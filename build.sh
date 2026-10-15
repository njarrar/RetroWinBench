#!/bin/sh
# Build winbench.exe with MinGW-w64 (i686). No C runtime is linked, and the
# program only imports functions that Windows 98 provides.
set -e
cd "$(dirname "$0")"
python3 tools/gen_data.py
CC=${CC:-i686-w64-mingw32-gcc}
WINDRES=${WINDRES:-i686-w64-mingw32-windres}
$WINDRES -I . src/winbench.rc -O coff -o src/winbench.res
CFLAGS="-O2 -march=i486 -mtune=i686 -mfpmath=387 -fno-builtin -ffreestanding -fno-stack-protector -fno-asynchronous-unwind-tables -fno-ident -Wall -Wno-parentheses"
$CC $CFLAGS -o winbench.exe src/main.c src/sysinfo.c src/cpu.c src/mem.c src/disk.c src/gpu.c src/score.c src/rt.c src/data.c src/winbench.res \
  -nostdlib -mwindows -Wl,-e,_wb_start@0 -Wl,--subsystem,windows:4.0 -Wl,--major-os-version,4 -Wl,--minor-os-version,0 \
  -Wl,--major-image-version,1 -Wl,--minor-image-version,0 -Wl,--disable-dynamicbase -Wl,--disable-nxcompat -Wl,--no-insert-timestamp -s \
  -lkernel32 -luser32 -lgdi32 -ladvapi32 -lgcc
python3 tools/check_imports.py winbench.exe
