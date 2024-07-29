qemu-system-x86_64 -cpu qemu64,+apic -smp 2 -M hpet=on -S -s -d int -no-reboot -monitor stdio -drive file=build/bootable.iso,format=raw,index=0,media=disk
