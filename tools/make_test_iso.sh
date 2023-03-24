
./mbr_maker.out test.iso 271520 boot.bin
echo '[DONE] mbr made'
./fat32_maker.out volume_size=271520 file=test.iso file_offset=1
