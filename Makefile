ROOT := /home/dscao/build/kernel/klibc/khdr
KINCL := /usr/lib/klibc/include


DEFS := -D__KLIBC__=2 -D__KLIBC_MINOR__=0 -D_BITSIZE=64 

COPTS := -nostdinc -fno-stack-protector -fwrapv -m64 -O2 -fomit-frame-pointer \
	-mno-sse -falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-asynchronous-unwind-tables -W -Wall -Wno-sign-compare \
	-Wno-unused-parameter -g

INCLS := -isystem $(ROOT)$(KINCL)/arch/x86_64 \
	-isystem $(ROOT)$(KINCL)/bits64 \
	-isystem $(ROOT)$(KINCL) \
	-isystem $(ROOT)/usr/include \
	-isystem /usr/lib/gcc/x86_64-linux-gnu/4.9/include

CFLAGS := $(COPTS) $(INCLS) $(DEFS)

.PHONEY: umall clean

umall: umall.o
	ld -m elf_x86_64 -e main $^ $(ROOT)/usr/lib/klibc/lib/interp.o \
		--start-group -R $(ROOT)/usr/lib/klibc/lib/libc.so \
		/usr/lib/gcc/x86_64-linux-gnu/4.9/libgcc.a --end-group \
		-o $@

clean:
	rm -f umall
	rm -f *.o
