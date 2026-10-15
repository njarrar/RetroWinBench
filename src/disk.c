/* Disk test: sequential write, sequential read and random 4 KB reads on a
   temporary file. Reads skip the Windows file cache where the system
   allows it; otherwise the file is made larger than RAM. */
#include "wb.h"

#define BLOCK (256 * 1024)

static HANDLE open_file(const char *path, int write, int nobuf)
{
    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    if (nobuf) flags |= FILE_FLAG_NO_BUFFERING;
    if (write) flags |= FILE_FLAG_WRITE_THROUGH;
    return CreateFileA(path, write ? GENERIC_WRITE : GENERIC_READ, 0, 0,
                       write ? CREATE_ALWAYS : OPEN_EXISTING, flags, 0);
}

void bench_disk(DiskResult *r, const char *dir, double free_mb, unsigned ram_mb, ProgressFn p)
{
    char path[MAX_PATH];
    unsigned char *buf;
    HANDLE h;
    unsigned mb, blocks, i, n;
    DWORD done;
    double t0, t;
    char msg[96];

    r->ok = 0;
    rt_fmt(path, sizeof path, "%s%sWBENCH.TMP", dir,
           dir[0] && dir[strlen(dir) - 1] == '\\' ? "" : "\\");

    /* try to bypass the cache; if the system refuses, use a file bigger than RAM */
    h = open_file(path, 1, 1);
    r->unbuffered = (h != INVALID_HANDLE_VALUE);
    if (!r->unbuffered) h = open_file(path, 1, 0);
    if (h == INVALID_HANDLE_VALUE) { rt_fmt(r->err, sizeof r->err, "cannot create %s", path); return; }

    mb = r->unbuffered ? 128 : ram_mb + ram_mb / 2;
    if (mb < 64) mb = 64;
    if (mb > 1024) mb = 1024;
    while (mb > 8 && mb + 50 > free_mb) mb /= 2;
    if (mb + 20 > free_mb) {
        CloseHandle(h); DeleteFileA(path);
        rt_fmt(r->err, sizeof r->err, "not enough free space in %s", dir); return;
    }
    r->file_mb = mb;
    blocks = mb * (1024 * 1024 / BLOCK);

    buf = rt_alloc(BLOCK);
    if (!buf) { CloseHandle(h); DeleteFileA(path); strcpy(r->err, "out of memory"); return; }
    for (i = 0; i < BLOCK / 4; i++) ((unsigned *)buf)[i] = rt_rand();

    /* sequential write */
    rt_fmt(msg, sizeof msg, "Disk: writing a %u MB test file...", mb); p(msg);
    t0 = now_sec();
    for (i = 0; i < blocks; i++) {
        if (!WriteFile(h, buf, BLOCK, &done, 0) || done != BLOCK) {
            CloseHandle(h); DeleteFileA(path); rt_free(buf);
            strcpy(r->err, "write failed"); return;
        }
        if ((i & 3) == 3) {
            p(0);
            if (r->unbuffered && i >= 63 && now_sec() - t0 >= 4.0) { i++; break; }
        }
    }
    blocks = i;
    r->file_mb = (blocks * (BLOCK / 1024)) / 1024;
    FlushFileBuffers(h);
    t = now_sec() - t0;
    CloseHandle(h);
    r->write_mbs = (double)blocks * (BLOCK / (1024.0 * 1024.0)) / t;

    /* sequential read */
    p("Disk: reading it back...");
    h = open_file(path, 0, r->unbuffered);
    if (h == INVALID_HANDLE_VALUE) { DeleteFileA(path); rt_free(buf); strcpy(r->err, "cannot reopen file"); return; }
    t0 = now_sec();
    for (i = 0; i < blocks; i++) {
        if (!ReadFile(h, buf, BLOCK, &done, 0) || done != BLOCK) break;
        if ((i & 3) == 3) {
            p(0);
            if (r->unbuffered && i >= 63 && now_sec() - t0 >= 4.0) { i++; break; }
        }
    }
    t = now_sec() - t0;
    r->read_mbs = (double)i * (BLOCK / (1024.0 * 1024.0)) / t;

    /* random 4 KB reads: about two seconds or 400 reads */
    p("Disk: random access...");
    n = 0; t0 = now_sec();
    do {
        unsigned off = (rt_rand() % (blocks * (BLOCK / 4096u))) * 4096u;
        SetFilePointer(h, (LONG)off, 0, FILE_BEGIN);
        ReadFile(h, buf, 4096, &done, 0);
        n++;
        t = now_sec() - t0;
        if ((n & 15) == 0) p(0);
    } while (t < 2.0 && n < 400);
    r->access_ms = t * 1000.0 / n;

    CloseHandle(h);
    DeleteFileA(path);
    rt_free(buf);
    r->ok = 1;
}
