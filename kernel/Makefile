CROSS_PATH=$${HOME}/cross/bin
CC=${CROSS_PATH}/x86_64-elf-gcc
AS=${CROSS_PATH}/x86_64-elf-as
CFLAGS =-mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -std=gnu99 -ffreestanding -Wall -Wextra -g -Os -fstack-protector-strong
LINKERFLAGS = -T linker.ld -ffreestanding -nostdlib -lgcc -no-pie -z max-page-size=4096

EXEC = krnl.bin

ARCH = x86-64
FIRMWARE_SUBSYS = bios

CSRC = $(shell find $(SOURCEDIR) -name '*.c')
SSRC = $(shell find $(SOURCEDIR) -name '*.s')

OBJDIR := obj
COBJS = $(CSRC:.c=.o)
SOBJS = $(SSRC:.s=.o)
OBJS = $(addprefix ${OBJDIR}/, ${COBJS}) $(addprefix ${OBJDIR}/, ${SOBJS})

FONTFILE := font/Tamsyn10x20r.psf

${EXEC}: ${OBJDIR} ${OBJS}
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

$(OBJS): | $(OBJDIR)

${OBJDIR}:
	mkdir ${OBJDIR}

clean:
	rm ${EXEC} ${OBJS} krnl.sym
