cd boot
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

SIZE=71520
./tools/mbr_maker.out build/bootable.iso "$SIZE" boot/first_stage_boot.bin 
./tools/fat32_maker.out volume_size="$SIZE" file=build/bootable.iso file_offset=1
