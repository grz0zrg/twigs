
# Twigs by The Orz
### 512 bytes Linux procedural graphics for Inércia Demoparty 2020

this doesn't use any libraries (not even the standard C library) and use two syscalls (open / mmap) to output to the Linux framebuffer device (fbdev), the magic lie in the code organization, symmetry, algorithms used (IFS Fractal / chaos game), GCC optimization flags and the compression stub / self executing shell script (without: 551 bytes)

goal was to port one of my IFS procedural gfx sketched in Processing as a C only 512 bytes linux intro

side goal was to serve as a proof of concept that many of my [2D IFS sketches](https://github.com/grz0zrg/Computer-Graphics) can be reduced to a <= 512 bytes program. I believe it can be done in less than 256 bytes also with agressive optimizations like ELF headers reduction or on different platform which doesn't have OS clutters like DOS with [Mode 13h](https://en.wikipedia.org/wiki/Mode_13h)

prototyped first with my [software graphics library](https://github.com/grz0zrg/fbg)

the party version in `party` directory was the original release and use floats but does not use saturation arithmetic so the image accumulate noise

the improved version in `party_improved` directory is compiled with gcc-12 (with -Oz flag instead of -Os) and is tailored for 1920x1080, 32 bits and use saturation arithmetic done with SSE2 instructions and fixed point arithmetic instead of floats (quick conversion with a stripped down and modified [fixedptc](https://sourceforge.net/projects/fixedptc/) ), the RNG can also use the hardware RNG (rdrand) although there is not much gain in quality or size, this version also doesn't draw outside screen boundary (the party version just limited the index so there was a single white pixel in the lower right corner), this version is also smaller overall with 491 bytes for the final binary

the 100% ELF version in `src` (and root binary) is like the improved version except it use a custom 32 bits ELF header with overlap and the fbdev setup is done in assembly, it does not rely anymore on a compression stub / self executing shell script and the final binary is **426 bytes** with rdrand instruction.

Note : party version 32 bits binary (-m32) is slightly bigger than the 64 bits binary which is 504 bytes (1920x1080x32)

requirements:
* 32 bits /dev/fb0 (framebuffer) with supported resolution
* high quality: the framebuffer should be set to display resolution

how to run:
* switch to console with Ctrl+Alt+F2 (Ctrl+Alt+F1 to switch back to X)
* check /dev/fb0 is writable to you (or launch with sudo to bypass this step)
* check /dev/fb0 current resolution with fbset tool
* run the appropriate binary

permission issue: add current user to either 'tty' or 'video' group or use 'sudo'

change graphical terminal resolution:
* check supported resolution with "vbeinfo" in GRUB command prompt
* "GRUB_GFXMODE=WxH" in /etc/default/grub (where W/H is a value)
* sudo update-grub

the single line RNG come from [here](https://blog.demofox.org/2013/07/07/a-super-tiny-random-number-generator/)

code by grz, party code built with GCC 7.5.0

greetings to all sizecoders