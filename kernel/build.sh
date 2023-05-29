PATH=$PATH:/home/simonem/cross/bin/
x86_64-elf-gcc -mcmodel=large -mno-red-zone -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c kmain.c -o kmain.o
x86_64-elf-as early_kmain.s -o early_kmain.o
x86_64-elf-gcc -T linker.ld -o krnl.bin -ffreestanding -O2 -nostdlib kmain.o early_kmain.o -lgcc
