#!/bin/sh
verify () {
	if [ ! -f "$1" ]; then
		echo ""                                                   1>&2
		echo " *** Missing file: $1"                              1>&2
		echo ' *** You need to run "make" before "make install".' 1>&2
		echo ""                                                   1>&2
		exit 1
 	fi
}

# Make sure the files actually exist
verify "$2"
verify "$3"

# User may have a custom install script

if [ -x ~/bin/${CROSS_COMPILE}installkernel ]; then exec ~/bin/${CROSS_COMPILE}installkernel "$@"; fi
if which ${CROSS_COMPILE}installkernel >/dev/null 2>&1; then
	exec ${CROSS_COMPILE}installkernel "$@"
fi

# Default install - same as make zlilo

back_it_up() {
	local file=$1
	[ -f ${file} ] || return 0
	local stamp=$(stat -c %Y ${file} 2>/dev/null)
	mv ${file} ${file}.${stamp:-old}
}

back_it_up $4/uImage
back_it_up $4/System.map

cat $2 > $4/uImage
cp $3 $4/System.map
