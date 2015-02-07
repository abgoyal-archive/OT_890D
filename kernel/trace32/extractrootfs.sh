#!/bin/sh -e
ROOTFS_GZ="rd6516-devel.gz"

if [ "$1" = "ttsystem" ]; then
	./ttimgextract/ttimgextract ttsystem
	ROOTFS_GZ="ttsystem.0"
elif [ "$1" != "" ]; then
	ROOTFS_GZ=$1
fi

echo "Extracting rootfs from '$ROOTFS_GZ'"

rm -rf rootfs
mkdir rootfs
gunzip -dc $ROOTFS_GZ > rootfs.cpio
cd rootfs
cat ../rootfs.cpio | sudo cpio -i --owner=`whoami`
rm ../rootfs.cpio
