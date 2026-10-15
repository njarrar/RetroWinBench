#!/bin/sh
# Build the scoring test and run it under Wine.
set -e
cd "$(dirname "$0")/.."
python3 tools/gen_data.py
i686-w64-mingw32-gcc -O2 -march=i486 -mfpmath=387 -fno-builtin -ffreestanding -fno-stack-protector -Wall -Wno-parentheses \
  -o tests/score_test.exe tests/score_test.c src/score.c src/rt.c src/data.c \
  -nostdlib -Wl,-e,_test_start@0 -lkernel32 -luser32 -lgcc
cd tests && WINEDEBUG=-all wine score_test.exe && head -45 t_ref.txt
