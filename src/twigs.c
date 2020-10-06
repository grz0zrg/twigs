#include <sys/mman.h>
#include <fcntl.h>
#ifdef __x86_64
    #include <asm/unistd_64.h>
#else
    #include <asm/unistd_32.h>
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))

// framebuffer settings
//#define FRAMEBUFFER_WIDTH 1920
//#define FRAMEBUFFER_HEIGHT 1080
#define FRAMEBUFFER_COMPONENTS 4
#define FRAMEBUFFER_LENGTH (FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * FRAMEBUFFER_COMPONENTS)

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
    __asm__ volatile("push %%ebp\nmov %0, %%ebp\nint $0x80\npop %%ebp\n" : : "a" (__NR_mmap), "b" (addr), "c" (length), "d" (prot), "S" (flags), "D" (fd) : "memory");
#endif
}

unsigned int getPixelIndex(unsigned int x, unsigned int y) {
    return (x + y * FRAMEBUFFER_WIDTH);
}

void _start() {
    int fbfd = sys_open("/dev/fb0", O_RDWR, 0);
    unsigned int *framebuffer = (unsigned int *)sys_mmap(0, FRAMEBUFFER_LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd);

    const int ifs = 4; // IFS count

    // IFS states
    float ifs_x[ifs];
    float ifs_y[ifs];

    unsigned int rng; // random number

    unsigned int i1; // framebuffer index which is also used as an iterator

    for(;;) {
        rng += (rng * rng) | 5; // random number generator

        unsigned int ifs_function = rng % 3; // IFS function selection

        unsigned int j = 1 + (i1 % 4); // IFS selection

        // save states
        float x = ifs_x[j];
        float y = ifs_y[j];

        // IFS core
        float ij = j * (IMAGE_WIDTH * 0.11);
        if (ifs_function == 0) {
            ifs_x[j] = y / 2.5;
            ifs_y[j] = 0.15 * IMAGE_WIDTH + x;
        } else if (ifs_function == 1) {
            ifs_x[j] = x / 1.6 + ij;
            ifs_y[j] = 0.55 * IMAGE_HEIGHT + 0.35 * x - 0.275 * y;//0.55 * (x / 1.6 + IMAGE_HEIGHT - 0.5 * y);
        } else if (ifs_function == 2) {
            ifs_x[j] = (0.5 * IMAGE_WIDTH * (ij + 0.5 * IMAGE_WIDTH - x)) / (IMAGE_WIDTH + x);
            ifs_y[j] = y / 1.25;
        }
        //

        // transforms & plot
        unsigned int x1 = ifs_x[j];
        unsigned int y1 = ifs_y[j] * j;

        i1 = MIN(getPixelIndex(OFFSET_X + x1, IMAGE_HEIGHT - y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        unsigned int i2 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH - x1, IMAGE_HEIGHT - y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        unsigned int i3 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH * 0.285 + ifs_x[j] * 2, y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        unsigned int i4 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH - ifs_x[j] * 2 - IMAGE_WIDTH * 0.285, y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);

        int color = 65793*2; // white
        if (j == 1) {
            color = 256; // green
        }

        framebuffer[i1] += color;
        framebuffer[i2] += color;
        framebuffer[i3] += color;
        framebuffer[i4] += color;
    }
}
