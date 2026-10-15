RetroWinBench 1.1
=================

One small program (winbench.exe, about 58 KB) that tests the CPU, RAM,
disk and 3D card of a Windows 98 or Windows XP PC, sums it up in one
score, compares that score with period hardware, and lists which classic
games the PC can run.

It needs no installer, no internet and no extra DLLs. Copy it to the PC
from a floppy, CD or USB stick and double-click it. It also runs on
Windows 95 OSR2, ME, 2000 and later.


HOW TO USE
----------
1. Close other programs.
2. Run winbench.exe and press "Run benchmark".
3. Wait one to three minutes. Do not touch the PC while it tests.
   A small 640x480 window opens for the 3D test.
4. Read the results. The program also saves them to winbench-report.txt
   next to the .exe (or in the TEMP folder if that folder is read-only).

Run "winbench.exe /auto" to test, save the report and close without
any clicks. Run "winbench.exe /run" to start the test at once
and keep the window open.


WHAT IT TESTS
-------------
CPU    Dhrystone 2.1 (whole numbers) and Whetstone (decimal maths).
       These are the classic programs, so the results line up with
       published figures for old PCs. Each runs for about 2 seconds,
       on one core, since games of this era used one core.
RAM    Read, write and copy speed on a block of up to 16 MB, larger
       than any CPU cache of the time.
Disk   Writes a test file to the TEMP folder (128 MB, or a file larger
       than RAM if Windows will not skip its file cache; slow drives
       stop after about 4 seconds each way), reads it back,
       then times random 4 KB reads. The file is deleted at the end.
3D     Uses OpenGL, which ships with Windows 98 and XP. Measures how
       many textured pixels and how many small triangles per second the
       card draws. If the PC has no 3D driver, Windows draws in software
       and the score shows it.

It also reads the Windows version, the DirectX version, the CPU name
and speed, the amount of RAM, the video card name and the free disk
space.


THE SCORE
---------
Each part gets a score where 100 equals the reference PC, a good PC from
2000 to 2001:

    Pentium III 1000 MHz, PC133 SDRAM, 7200 rpm ATA/100 disk,
    GeForce2 GTS

A score of 200 means twice as fast as that PC. The total weights the
parts the way they matter for games: CPU 35%, 3D 35%, RAM 15%, disk 15%
(a weighted geometric mean, so one very fast part cannot hide a slow one).

The report then shows:
  - the CPU in the built-in table closest to your CPU score
  - how your CPU and 3D card rank against the table
  - the total score of a typical new mid-range PC from each year
    from 1997 to 2008, and which year your PC matches


GAMES
-----
The program holds the box requirements of 135 Windows games from 1996
to 2009. For each game it checks the Windows version, CPU score, RAM, 3D
card and, for DirectX 8 and later games, whether the card has the
shader hardware the game needs. Games land in three groups:

  Runs well   meets the recommended specs
  Runs        meets the minimum specs; it says what holds it back
  Will not    below the minimum; it says why

It also says when a game needs a newer DirectX (a free install) or more
free disk space. A * after the year marks games whose specs in the table
include estimates.

For the 3D card the game check uses the card's rating from the table
when the program knows the card by name, since that rating comes from
game benchmarks. If it does not know the card, it uses the measured 3D
score. A 3dfx Voodoo or Voodoo2 add-on card is found through its Glide
driver files.


WHERE THE NUMBERS COME FROM
---------------------------
All tables are plain CSV files in the data folder and go into the
program when it is built. Each row has an "est" column: 1 means the
figure is an estimate rather than a value read from a published source.

data/refs.csv       Reference PC figures.
                    Sourced: Pentium III 1000 Dhrystone 2.1 (1595
                    DMIPS) and Whetstone (816 MWIPS), from Roy
                    Longbottom's PC benchmark results.
                    Estimated: RAM, disk and 3D reference figures,
                    from period reviews (Sandra, 3DMark2001).
data/cpus.csv       122 CPUs from 1996 to 2008, rated against the
                    Pentium III 1000. 16 ratings come straight from
                    Longbottom's Dhrystone and Whetstone results; the
                    rest scale those by clock speed and chip design
                    (marked est=1).
data/gpus.csv       152 3D cards, rated against the GeForce2 GTS from
                    period game benchmarks. All are estimates.
data/games.csv      135 games with minimum and recommended specs. 120
                    were checked against PCGamingWiki or
                    gamesystemrequirements.com (the src column names
                    the site); the other 15 are marked est=1. Some
                    install sizes are today's Steam figures, larger
                    than the original box.
data/years.csv      Typical new PC for each year (estimates from period
                    ads and buying guides).
data/reference.txt  Notes, figures and links behind all of the above.

Main sources: Roy Longbottom's Dhrystone and Whetstone result tables
(roylongbottom.org.uk), Guru3D and Hardware Secrets memory bandwidth
results, Tom's Hardware, Wikipedia GPU spec lists, PCGamingWiki.

To correct a figure, edit the CSV file and rebuild.


LIMITS
------
- The Dhrystone and Whetstone results depend on the compiler. This
  build uses GCC; Longbottom's figures used older compilers, so CPU
  scores can be off by about 20%.
- Some systems (and Wine) ignore the request to skip the file cache.
  If that happens the report says so and the disk numbers are too high.
- The 3D test uses OpenGL. A card with a poor OpenGL driver, which was
  common on Windows 98, will score lower than it does in Direct3D games.
  The game list uses the card's table rating when it knows the card,
  which avoids this.
- The RAM test uses at most a quarter of RAM so the PC does not swap.
  On a PC with less than 128 MB it tests a smaller block, still larger
  than the CPU cache.


BUILDING
--------
On Linux with MinGW-w64 (tested with i686-w64-mingw32-gcc 13) and
Python 3:

    sh build.sh

This turns the CSV files into src/data.c, compiles winbench.exe for any
CPU from the 486 up (x87 maths, no SSE), and links it with no C runtime.
It then runs tools/check_imports.py, which fails the build if the .exe
calls any Windows function that Windows 98 does not have. Newer
functions (EnumDisplayDevicesA, GetDiskFreeSpaceExA) and OpenGL are
looked up at run time instead.

tests/run_tests.sh feeds made-up results for three known PCs into the
scoring code under Wine and writes their reports, to check the scores
and game lists.

Files:
  src/main.c      window, buttons, order of tests
  src/sysinfo.c   Windows, DirectX, CPU, RAM, video card, disk space
  src/cpu.c       Dhrystone 2.1 and Whetstone
  src/mem.c       RAM test
  src/disk.c      disk test
  src/gpu.c       OpenGL 3D test
  src/score.c     scores, comparisons, game checks, report
  src/rt.c        small replacement for the C runtime
  tools/          data generator and import checker
