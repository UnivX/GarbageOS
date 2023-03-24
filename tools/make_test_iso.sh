
./mbr_maker.out test.iso 71520 boot.bin
echo '[DONE] mbr made'
./fat32_maker.out volume_size=71520 file=test.iso file_offset=1
