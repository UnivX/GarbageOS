CROSS_PATH=$${HOME}/cross/bin
CC=${CROSS_PATH}/x86_64-elf-gcc
AS=${CROSS_PATH}/x86_64-elf-as
NASM=nasm
CFLAGS =-mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -std=gnu11 -ffreestanding -Wall -Wextra -g -O3 -fstack-protector-strong
LINKERFLAGS = -T linker.ld -ffreestanding -nostdlib -lgcc -no-pie -z max-page-size=4096

EXEC = krnl.bin

ARCH = x86-64
FIRMWARE_SUBSYS = bios

CSRC = $(shell find $(SOURCEDIR) -name '*.c')
SSRC = $(shell find $(SOURCEDIR) -name '*.s')
BINASMSRC = $(shell find $(SOURCEDIR) -name '*.bin_asm')

OBJDIR := obj
COBJS = $(CSRC:.c=.o)
SOBJS = $(SSRC:.s=.o)
BINOBJS = $(BINASMSRC:.bin_asm=.o)
RAW_OBJS = $(addprefix ${OBJDIR}/, ${COBJS}) $(addprefix ${OBJDIR}/, ${SOBJS}) $(addprefix ${OBJDIR}/, ${BINOBJS})
OBJS = $(subst /./,/,${RAW_OBJS})


FONTFILE := font/Tamsyn10x20r.psf

${EXEC}: ${OBJDIR} ${OBJS}
	echo "${OBJS}"
	echo "end"
	ld -r -b binary -o ${OBJDIR}/font.o ${FONTFILE}
	${CC} ${LINKERFLAGS} ${OBJS} ${OBJDIR}/font.o -o ${EXEC}
	objcopy --only-keep-debug ${EXEC} krnl.sym
	objcopy --strip-debug ${EXEC}


${OBJDIR}/%.o : %.c
	@mkdir -p $(@D)
	${CC} ${CFLAGS} -c $< -o $@

${OBJDIR}/%.o : %.s
	@mkdir -p $(@D)
	${AS} -o $@ $<
	
${OBJDIR}/%.o : %.bin_asm
	@mkdir -p $(@D)
	${NASM} $< -f bin -o $@_assembled
	ld -r -b binary -o $@ $@_assembled

$(OBJS): | $(OBJDIR)

${OBJDIR}:
	mkdir -p ${OBJDIR}

clean:
	rm ${EXEC} ${OBJS} krnl.sym
