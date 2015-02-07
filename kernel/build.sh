#!/bin/sh
verfile="android.ver"
defcfg="config-default"
curcfg=".config"
release="n"
rebuild="n"
clean="n"
makeflags="-w"
makedefs="V=0"
makejobs=""

usage() {
    echo "Usage: $0 {release|rebuild|clean|silent|verbose|single} config-xxx"
    exit 1
}

make_clean() {
    echo "**** Cleaning..."
    nice make ${makeflags} ${makedefs} distclean
}

make_config() {
    if test ! -f "$defcfg"; then
		usage
		exit 1
	fi
    echo "**** Configuring with ${defcfg}..."

    cat "${defcfg}" > "${curcfg}"
    echo "**** Configured ****"
    if [ x$HAVE_AEE_FEATURE = xyes ]; then
	# Enable AEE module & set panic 5 sec timeout.
	sed --in-place=.orig \
	    -e 's/# CONFIG_AEE_FEATURE is not set/CONFIG_AEE_FEATURE=y/' \
	    -e 's/\(CONFIG_PANIC_TIMEOUT=\).*/\15/' \
	    .config
    fi
    nice make ${makeflags} ${makedefs} silentoldconfig
}

make_build() {
    echo "**** Building..."
    make ${makeflags} ${makejobs} ${makedefs}
    if [ $? -ne 0 ]; then
      exit 1;
    fi
}


# Main starts here
while test -n "$1"; do
    case "$1" in
    release)
        release="y"
    ;;
    rebuild)
        rebuild="y"
    ;;
    clean)
        clean="y"
    ;;
    silent)
        makeflags="-ws"
        makedefs="V=0"
    ;;
    verbose)
        makeflags="-w"
        makedefs="V=1"
    ;;
    single)
        makejobs=""
    ;;
    config-*)
        config_file=$1
        if [[ ${TARGET_BUILD_VARIANT}xx = userxx ]]; then
            config_file=${config_file}-user
        fi
        if test ! -f "$config_file"; then
            die "$config_file does not exist!"
        fi
        if test -f "${curcfg}" && ! rm -f "${curcfg}"; then
            die "Unable to remove ${curcfg}!"
        fi
        defcfg="$config_file"
    ;;
    *)
        usage
    ;;
    esac
    shift
done

    id2=`echo $defcfg | sed -e 's/config-mt6516-//g' | sed -e 's/-.*//g'`
    case "$id2" in
    evb)
        export MTK_PROJECT=mt6516_evb
    ;;
    phone)
        export MTK_PROJECT=gw616
    ;;
    gemini)
        export MTK_PROJECT=ds269
    ;;
    oppo|e1k|e1kv2|simcom16_bu1_a10x|bird16_a10x|vigor16_a10x|htt16_a10x|oppo16_a10x|jrd16_a10x|agold16_a10x|zte16_a10x|uniscope16_a10x|qishang16_a10x|shcking16_a10x|lcsh16_a10x|simcom16_bu2_a10x)
        export MTK_PROJECT=$id2
    ;;
    *)
        usage
    ;;
    esac
    if [ ! -e "${MTK_PROJECT}_mtk_cust.mak" ]; then
    cd ..
    perl ./makeMtk $MTK_PROJECT gen_cust
    cd kernel
    fi

if test "${rebuild}" = "y"; then
    make_clean
    make_config
elif test "${clean}" = "y"; then
    make_clean
    echo "**** Successfully cleaned kernel ****"
    exit 0
elif test ! -f "${curcfg}" -o "${defcfg}" -nt "${curcfg}"; then
    make_config
fi

make_build

echo "**** Successfully built kernel ****"

# Infinity 20090204 {
curdir=`pwd`
imgdir="./Download"
id=`echo $defcfg | sed -e 's/config-mt*//g' | sed -e 's/-.*//g'`
sdimgdir="${imgdir}/sdcard"
flimgdir="${imgdir}/flash"
kernel_img="${curdir}/arch/arm/boot/Image"
kernel_zimg="${curdir}/arch/arm/boot/zImage"
mkimg="${curdir}/trace32/tools/mkimage"
rootfs_img="${curdir}/trace32/rd${id}-devel.gz"

echo "**** Generate download images to ${imgdir} ****"

# create folders
if [ ! -d ${imgdir} ]; then
    mkdir ${imgdir}
fi
if [ ! -d ${sdimgdir} ]; then
    mkdir ${sdimgdir}
fi
if [ ! -d ${flimgdir} ]; then
    mkdir ${flimgdir}
fi

if [ ! -x ${mkimg} ]; then
    chmod a+x ${mkimg}
fi

# copy kernel and rootfs images
if [ -e ${kernel_img} ]; then
    ln -sf ${kernel_img} ${sdimgdir}/Image
fi

if [ -e ${rootfs_img} ]; then
    ln -sf ${rootfs_img} ${sdimgdir}/rd${id}.gz
fi

#${mkimg} ${kernel_img} KERNEL > ${flimgdir}/${MTK_PROJECT}_kernel.bin
${mkimg} ${kernel_zimg} KERNEL > ${flimgdir}/${MTK_PROJECT}_kernel.bin
${mkimg} ${rootfs_img} ROOTFS > ${flimgdir}/${MTK_PROJECT}_rootfs.bin

cd ${flimgdir}
if [ -e kernel.bin ]; then
    rm kernel.bin
fi
if [ -e rootfs.bin ]; then
    rm rootfs.bin
fi
ln -s ${MTK_PROJECT}_kernel.bin kernel.bin
ln -s ${MTK_PROJECT}_rootfs.bin rootfs.bin

# Infinity 20090204 }
