#include <x86intrin.h>
#include <stdint.h>

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

unsigned int getPixelIndex(unsigned int x, unsigned int y) {
    return (x + y * FRAMEBUFFER_WIDTH);
}

void _start() {
#ifdef __i386__
    register unsigned int *framebuffer __asm__("eax");
#endif

    const int ifs = 4; // IFS count

    // IFS states
    fixedpt ifs_x[ifs];
    fixedpt ifs_y[ifs];

    unsigned int rng; // random number

    unsigned int i1; // framebuffer index which is also used as an iterator

    for(;;) {
        // still good quality but actually bigger binary; use this instead of rdrand for compat
        //rng += (rng * rng) | 5; // random number generator
        int f;
        do { __asm__ __volatile__("rdrand %1" : "=@ccc"(f), "=r"(rng)); } while (!f);

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
