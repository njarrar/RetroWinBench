/* RAM test: read, write and copy speed on a block much larger than the
   CPU cache, so the numbers reflect the memory chips and bus. */
#include "wb.h"

volatile unsigned g_mem_sink;

static unsigned read_block(const unsigned *p, unsigned words)
{
    unsigned a = 0, b = 0, c = 0, d = 0, i;
    for (i = 0; i < words; i += 8) {
        a += p[i]; b += p[i + 2]; c += p[i + 4]; d += p[i + 6];
        a ^= p[i + 1]; b ^= p[i + 3]; c ^= p[i + 5]; d ^= p[i + 7];
    }
    return a + b + c + d;
}

static void write_block(unsigned *p, unsigned words, unsigned v)
{
    __asm__ volatile("cld\n\trep stosl" : "+D"(p), "+c"(words) : "a"(v) : "memory");
}

static void copy_block(unsigned *d, const unsigned *s, unsigned words)
{
    __asm__ volatile("cld\n\trep movsl" : "+D"(d), "+S"(s), "+c"(words) : : "memory");
}

static double timed(int kind, unsigned *a, unsigned *b, unsigned bytes)
{
    unsigned words = bytes / 4, reps = 0;
    double t0 = now_sec(), t;
    do {
        if (kind == 0) g_mem_sink += read_block(a, words);
        else if (kind == 1) write_block(a, words, reps);
        else copy_block(b, a, words);
        reps++;
        t = now_sec() - t0;
    } while (t < 1.0);
    return (double)bytes * reps / t / (1024.0 * 1024.0);
}

void bench_mem(MemResult *r, unsigned ram_mb, ProgressFn p)
{
    unsigned bytes = 16u << 20;
    unsigned *a, *b;

    /* stay well inside physical RAM on small machines */
    while (bytes > (1u << 20) && bytes / (1u << 20) * 8 > ram_mb) bytes >>= 1;
    r->block_kb = bytes / 1024;

    a = rt_alloc(bytes); b = rt_alloc(bytes);
    if (!a || !b) { rt_free(a); rt_free(b); return; }
    write_block(a, bytes / 4, 1); write_block(b, bytes / 4, 2);   /* touch every page first */

    p("RAM: read speed...");  r->read_mbs  = timed(0, a, b, bytes);
    p("RAM: write speed..."); r->write_mbs = timed(1, a, b, bytes);
    p("RAM: copy speed...");  r->copy_mbs  = timed(2, a, b, bytes);

    rt_free(a); rt_free(b);
}
