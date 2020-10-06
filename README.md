
# Twigs by The Orz
### 512 bytes Linux procedural graphics for Inércia Demoparty 2020

this doesn't use any libraries (not even the standard C library) and use two syscalls (open / mmap) to output to the Linux framebuffer device (fbdev), the magic lie in the code organization, algorithms used (IFS Fractal / chaos game), GCC optimization flags and the compression stub / self executing shell script (without: 728b)

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

goal was to port one of my IFS procedural gfx sketched in Processing as a C only 512b linux intro

code by grz and built with GCC 7.5.0

greetings to all sizecoders

28/09/2020

