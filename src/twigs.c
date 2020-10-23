#include <sys/mman.h>
#include <fcntl.h>
#ifdef __x86_64
    #include <asm/unistd_64.h>
#else
    #include <asm/unistd_32.h>
#endif

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

        unsigned int xx1 = OFFSET_X + x1;
        unsigned int xx2 = OFFSET_X + IMAGE_WIDTH - ifs_x[j] * 2 - IMAGE_WIDTH * 0.285;

        i1 = min(getPixelIndex(xx1, IMAGE_HEIGHT - y1), FRAMEBUFFER_BOUNDS);
        unsigned int i2 =  min(getPixelIndex(FRAMEBUFFER_WIDTH - xx1, IMAGE_HEIGHT - y1), FRAMEBUFFER_BOUNDS);
        unsigned int i3 =  min(getPixelIndex(FRAMEBUFFER_WIDTH - xx2, y1), FRAMEBUFFER_BOUNDS);
        // party code (note : optimized after)
        //unsigned int i2 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH - x1, IMAGE_HEIGHT - y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        //unsigned int i3 = MIN(getPixelIndex(OFFSET_X + IMAGE_WIDTH * 0.285 + ifs_x[j] * 2, y1), FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT-4);
        //
        unsigned int i4 = min(getPixelIndex(xx2, y1), FRAMEBUFFER_BOUNDS);

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
