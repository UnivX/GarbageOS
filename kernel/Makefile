CROSS_PATH=/home/simonem/cross/bin
CC=${CROSS_PATH}/x86_64-elf-gcc
AS=${CROSS_PATH}/x86_64-elf-as
CFLAGS =-mcmodel=large -mno-red-zone -std=gnu99 -ffreestanding -O2 -Wall -Wextra 
LINKERFLAGS = -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

EXEC = krnl.bin
OBJS = kmain.o vbe.o early_kmain.o

 

${EXEC}: ${OBJS}
	${CC} ${LINKERFLAGS} ${OBJS} -o ${EXEC}

%.o : %.c
	${CC} ${CFLAGS} -c -o $@ $<

%.o : %.s
	${AS} -o $@ $<

clean:
	rm ${OBJS} ${EXEC}
