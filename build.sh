if ! [ $(id -u) = 0 ]; then
   echo "run this as root" >&2
   exit 1
fi

if [ $SUDO_USER ]; then
    real_user=$SUDO_USER
else
    real_user=$(whoami)
fi

#run the sub_build script without root privileges
sudo -u $real_user sh sub_build.sh

mount -t vfat -o loop,offset=512 build/bootable.iso build/iso
mkdir build/iso/sys/
cp kernel/krnl.bin build/iso/sys
cd build/iso/sys
echo 'this message is being printed from a file as a test' > t.txt
cd ../../../
umount build/iso
