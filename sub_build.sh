cd boot
sh ./build.sh
cd ..

cd kernel
sh ./build.sh
cd ..

#compile the tools only if needed
MBR_MAKER_FILE="tools/mbr_maker.out"
if [ ! -f "$MBR_MAKER_FILE" ]
then
	cd tools
	sh ./build.sh
	cd ..
fi

if [ ! -f "build/iso" ]
then
	mkdir build
	mkdir build/iso
fi

SIZE=71520
./tools/mbr_maker.out build/bootable.iso "$SIZE" boot/first_stage_boot.bin 
./tools/fat32_maker.out volume_size="$SIZE" file=build/bootable.iso file_offset=1 boot_code=boot/second_stage_boot.bin

