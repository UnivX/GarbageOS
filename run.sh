qemu-system-x86_64 -no-shutdown -no-reboot -d int -display gtk,zoom-to-fit=on -cpu qemu64,+apic -smp 4 -M hpet=on -S -s -monitor stdio -drive file=build/bootable.iso,format=raw,index=0,media=disk
