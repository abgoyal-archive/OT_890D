#!/bin/sh -e
ROOTFS_GZ=rd6516-devel.gz

if [ "$1" != "" ]; then
	ROOTFS_GZ=$1
fi

ROOTFS=`echo $ROOTFS_GZ | awk -F. '{print $1}'`

cd rootfs
find . -print | sudo cpio -H newc -o > ../$ROOTFS
cd ..
rm -rf $ROOTFS.gz
gzip -9 $ROOTFS
