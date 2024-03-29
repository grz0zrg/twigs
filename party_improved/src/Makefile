CC=gcc-12
STANDARD_FLAGS=-std=c99 -pedantic -Wno-switch -Wno-conversion -Wno-uninitialized -Wno-strict-aliasing \
	-fno-exceptions -ffast-math -fsingle-precision-constant -fno-ident \
    -funsafe-math-optimizations -fvisibility=hidden -fmerge-all-constants \
    -fno-align-functions -fno-align-loops -fno-math-errno -march=ivybridge -m32
OPTI_FLAGS=-Oz -msse2 -fno-inline-functions -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector -ftree-vectorize -fopt-info-vec-optimized \
	-fomit-frame-pointer -fno-math-errno -fdata-sections -ffunction-sections -fno-stack-protector -fno-stack-check \
	-nostartfiles -nodefaultlibs -static -no-pie -fno-plt -fno-pic -fno-unroll-loops -Winline 
LINKER_FLAGS=-Wl,--gc-sections \
	-Wl,--build-id=none -z norelro -Wl,-z,noseparate-code -Wl,--no-eh-frame-hdr \
	-Wl,--no-ld-generated-unwind-info -Wl,--hash-style=sysv \
	-Wl,-z,nodynamic-undefined-weak 
NAME=twigs
# https://github.com/BR903/ELFkickers
SSTRIP=~/ELFkickers/sstrip/sstrip
LZMA_ARGS=--format=lzma -9 --extreme --lzma1=preset=9,lc=0,lp=0,pb=0,nice=32,depth=0,dict=16384 --keep

all:
	$(CC) -o $(NAME) $(NAME).c -DFRAMEBUFFER_WIDTH=$(FRAMEBUFFER_WIDTH) -DFRAMEBUFFER_HEIGHT=$(FRAMEBUFFER_HEIGHT) -DIMAGE_WIDTH=$(IMAGE_WIDTH) $(STANDARD_FLAGS) $(OPTI_FLAGS) $(LINKER_FLAGS)
	wc -c $(NAME)
	#norjohe $(NAME) $(NAME)
	strip -R .note -R .comment -R .eh_frame -R .eh_frame_hdr -R .note.gnu.build-id -R .got -R .got.plt -R .gnu.version -R .rela.dyn -R .shstrtab -R .gnu.hash $(NAME)
	# for some reasons -z option don't work sometimes (so ~13 bytes bigger than it shoud be !) alignment issue ?
	$(SSTRIP) $(NAME)
	# clear useless bits (taken from blackle Tiny X11 Trans Flag)
	sed -i 's/_edata/\x00\x00\x00\x00\x00\x00/g' $(NAME);
	sed -i 's/__bss_start/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g' $(NAME);
	sed -i 's/_end/\x00\x00\x00\x00/g' $(NAME);
	wc -c $(NAME)
	# compress & stub
	lzma $(LZMA_ARGS) $(NAME)
	cat unpack_lzma.header $(NAME).lzma > $(NAME)
	chmod +x $(NAME)
	rm $(NAME).lzma
	# remove CRC32 (4 bytes)
	truncate -s -4 $(NAME)
	# truncate some more bytes (NOTE : unsafe, upon any segfaults just comment the next line)
	#truncate -s -1 $(NAME)
	wc -c $(NAME)
	mv $(NAME) ../$(NAME)_$(FRAMEBUFFER_WIDTH)x$(FRAMEBUFFER_HEIGHT)x32
