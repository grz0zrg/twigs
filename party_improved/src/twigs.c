#include <sys/mman.h>
#include <fcntl.h>
#include <x86intrin.h>
#include <stdint.h>
#ifdef __x86_64
    #include <asm/unistd_64.h>
#else
    #include <asm/unistd_32.h>
#endif

// https://sourceforge.net/p/fixedptc/code/ci/default/tree/fixedptc.h
typedef int32_t fixedpt;
typedef	int32_t	fixedptd; // for accuracy in 64 bits -> int64_t
typedef	uint32_t fixedptu;
typedef	uint32_t fixedptud; // for accuracy in 64 bits -> uint64_t

#define FIXEDPT_BITS	32
#define FIXEDPT_WBITS	27 // for accuracy in 64 bits -> 24; otherwise higher value change the structure a bit
#define FIXEDPT_FBITS	(FIXEDPT_BITS - FIXEDPT_WBITS)

#define fixedpt_rconst(R) ((fixedpt)((R) * FIXEDPT_ONE + ((R) >= 0 ? 0.5 : -0.5)))
#define fixedpt_fromint(I) ((fixedptd)(I) << FIXEDPT_FBITS)
#define fixedpt_toint(F) ((F) >> FIXEDPT_FBITS)
#define FIXEDPT_ONE	((fixedpt)((fixedpt)1 << FIXEDPT_FBITS))
#define fixedpt_xmul(A,B)						\
	((fixedpt)(((fixedptd)(A) * (fixedptd)(B)) >> FIXEDPT_FBITS))
#define fixedpt_xdiv(A,B)						\
	((fixedpt)(((fixedptd)(A) << FIXEDPT_FBITS) / (fixedptd)(B)))
//

// framebuffer settings
//#define FRAMEBUFFER_WIDTH 1920
//#define FRAMEBUFFER_HEIGHT 1080
#define FRAMEBUFFER_COMPONENTS 4
#define FRAMEBUFFER_LENGTH (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * FRAMEBUFFER_COMPONENTS)
#define FRAMEBUFFER_BOUNDS (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT - 4)

// image settings
//#define IMAGE_WIDTH (FRAMEBUFFER_HEIGHT-180) // should be roughly equal to FRAMEBUFFER_HEIGHT - 180
#define IMAGE_HEIGHT (IMAGE_WIDTH + 120)
//
#define OFFSET_X (FRAMEBUFFER_WIDTH / 2 - IMAGE_WIDTH / 2)

inline static int sys_open(const char *filepath, int flags, int mode) {
    int r;
#ifdef __x86_64
    __asm__ volatile("syscall"
        : "=a"(r)
        : "0"(__NR_open), "D"(filepath), "S"(flags), "d"(mode)
        : "cc", "rcx", "r11", "memory");
#else
    __asm__ volatile("int $0x80"
        : "=a"(r)
        : "0"(__NR_open), "b"(filepath), "c"(flags), "d"(mode)
        : "cc", "edi", "esi", "memory");
#endif
  return r;
}

inline static unsigned int *sys_mmap(unsigned int *addr, unsigned long length, unsigned long prot, unsigned long flags, unsigned long fd) {
#ifdef __x86_64
    register volatile int r10 __asm__ ("r10") = flags; register volatile int r8 __asm__ ("r8") = fd; unsigned int *r;
    __asm__ volatile ("syscall" : "=a" (r) : "a" (__NR_mmap), "D" (addr), "S" (length), "d" (prot), "r" (r10), "r" (r8) : "cc", "memory", "r11", "rcx");
    return r;
#else
    unsigned int args[2] = { (unsigned int)addr, 0 };
    unsigned int *r;

    __asm__ __volatile__("push %%ebp\n"
                        "movl 4(%%ebx), %%ebp\n"
                        "movl 0(%%ebx), %%ebx\n"
                        "int $0x80\n"
                        "pop %%ebp\n"
                        : "=a"(r)
                        : "a"(__NR_mmap2), "b"(&args),
                            "c"(length), "d"(prot), "S"(flags), "D"(fd));
    return r;
#endif
}

unsigned int min(unsigned int a, unsigned int b) {
    return (a < b) ? a : b;
}

unsigned int getPixelIndex(unsigned int x, unsigned int y) {
    return (x + y * FRAMEBUFFER_WIDTH);
}

void _start() {
    int fbfd = sys_open("/dev/fb0", O_RDWR, 0);
    unsigned int *framebuffer = (unsigned int *)sys_mmap(0, FRAMEBUFFER_LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd);

    const int ifs = 4; // IFS count

    // IFS states
    fixedpt ifs_x[ifs];
    fixedpt ifs_y[ifs];

    unsigned int rng; // random number

    unsigned int i1; // framebuffer index which is also used as an iterator

    for(;;) {
        // without rdrand instruction; hardware rng (still good quality and ~3 bytes less unless the rdrand check / loop is removed)
        rng += (rng * rng) | 5; // random number generator
        //int f;
        //do { __asm__ __volatile__("rdrand %1" : "=@ccc"(f), "=r"(rng)); } while (!f);

        unsigned int ifs_function = rng % 3; // IFS function selection

        unsigned int j = 1 + (i1 % 4); // IFS selection

        // save states
        fixedpt x = ifs_x[j];
        fixedpt y = ifs_y[j];

        // IFS core
        fixedpt ij = fixedpt_fromint(j * 99);// 99 -> 900 * 0.11;
        if (ifs_function == 0) {
            ifs_x[j] = fixedpt_xdiv(y, fixedpt_rconst(2.5));
            ifs_y[j] = fixedpt_fromint(135) + x; // 135 -> 0.15 * 900
        } else if (ifs_function == 1) {
            ifs_x[j] = fixedpt_xdiv(x, fixedpt_rconst(1.6)) + ij;
            ifs_y[j] = fixedpt_rconst(0.55 * IMAGE_HEIGHT) + fixedpt_xmul(fixedpt_rconst(0.35), x) - fixedpt_xmul(fixedpt_rconst(0.275), y);
        } else if (ifs_function == 2) {
            ifs_x[j] = fixedpt_xdiv(fixedpt_xmul(fixedpt_rconst(0.5 * IMAGE_WIDTH), (ij + fixedpt_rconst(0.5 * IMAGE_WIDTH) - x)), (fixedpt_fromint(IMAGE_WIDTH) + x));
            ifs_y[j] = fixedpt_xdiv(y, fixedpt_rconst(1.25));
        }
        //

        // transforms & plot
        unsigned int x1 = fixedpt_toint(ifs_x[j]);
        unsigned int y1 = fixedpt_toint(fixedpt_xmul(ifs_y[j],fixedpt_fromint(j)));

        unsigned int xx1 = OFFSET_X + x1;
        unsigned int xx2 = fixedpt_toint(fixedpt_fromint(OFFSET_X + IMAGE_WIDTH) - fixedpt_xmul(ifs_x[j], fixedpt_fromint(2)) - fixedpt_rconst(IMAGE_WIDTH * 0.285));//(OFFSET_X + IMAGE_WIDTH - fixedpt_toint(ifs_x[j]) * 2) - 256; // 256 -> 900 * 0.285;

        i1 = getPixelIndex(xx1, IMAGE_HEIGHT - y1);
        unsigned int i2 = getPixelIndex(FRAMEBUFFER_WIDTH - xx1, IMAGE_HEIGHT - y1);
        unsigned int i3 = getPixelIndex(FRAMEBUFFER_WIDTH - xx2, y1);
        // party code (note : optimized after)
        //unsigned int i2 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH - x1, IMAGE_HEIGHT - y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        //unsigned int i3 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH * 0.285 + ifs_x[j] * 2, y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        //
        unsigned int i4 = getPixelIndex(xx2, y1);

        int color = 65793 * 2; // white
        if (j == 1) {
            color = 256; // green
        }

/*
        // party version (no saturation)
        framebuffer[i1] += color;
        framebuffer[i2] += color;
        framebuffer[i3] += color;
        framebuffer[i4] += color;
*/

        // saturation
        __m128i fv = _mm_set_epi32(color, color, color, color), t, v;
        if (i1 < FRAMEBUFFER_BOUNDS) {
            t = _mm_loadu_si128((const __m128i *)&framebuffer[i1]);
            v = _mm_adds_epu8(fv, t);
            _mm_storeu_si32((unsigned int*)&framebuffer[i1], v);
        }
        if (i2 < FRAMEBUFFER_BOUNDS) {
            t = _mm_loadu_si128((const __m128i *)&framebuffer[i2]);
            v = _mm_adds_epu8(fv, t);
            _mm_storeu_si32((unsigned int*)&framebuffer[i2], v);
        }
        if (i3 < FRAMEBUFFER_BOUNDS) {
            t = _mm_loadu_si128((const __m128i *)&framebuffer[i3]);
            v = _mm_adds_epu8(fv, t);
            _mm_storeu_si32((unsigned int*)&framebuffer[i3], v);
        }
        if (i4 < FRAMEBUFFER_BOUNDS) {
            t = _mm_loadu_si128((const __m128i *)&framebuffer[i4]);
            v = _mm_adds_epu8(fv, t);
            _mm_storeu_si32((unsigned int*)&framebuffer[i4], v);
        }
    }
}
