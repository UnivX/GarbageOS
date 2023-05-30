CROSS_PATH=/home/simonem/cross/bin
CC=${CROSS_PATH}/x86_64-elf-gcc
AS=${CROSS_PATH}/x86_64-elf-as
CFLAGS =-mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -std=gnu99 -ffreestanding -Os -Wall -Wextra -g
LINKERFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

EXEC = krnl.bin
OBJS = kmain.o vbe.o early_kmain.o memory.o misc.o

 

${EXEC}: ${OBJS}
	${CC} ${LINKERFLAGS} ${OBJS} -o ${EXEC}
	objcopy --only-keep-debug ${EXEC} krnl.sym
	objcopy --strip-debug ${EXEC}

%.o : %.c
	@echo "compiling $< ..."
	${CC} ${CFLAGS} -c -o $@ $<

%.o : %.s
	@echo "compiling $< ..."
	${AS} -o $@ $<

clean:
	rm ${OBJS} ${EXEC} krnl.sym
