qemu-system-x86_64 -display gtk,zoom-to-fit=on -cpu qemu64,+apic -smp 2 -M hpet=on -S -s -d int -no-reboot -monitor stdio -drive file=build/bootable.iso,format=raw,index=0,media=disk
