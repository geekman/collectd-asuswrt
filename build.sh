#!/bin/sh
#
# cross-compiles collectd for ASUSWRT
# call this script from the collectd source dir
#

set -e 

PKGVER=5.9.0
PKGURL="https://collectd.org/files/collectd-$PKGVER.tar.bz2"
SHA256SUM=7b220f8898a061f6e7f29a8c16697d1a198277f813da69474a67911097c0626b

OUT_TARBALL=collectd-$PKGVER.tar.gz

fakeroot -v >/dev/null 2>/dev/null || exit

if [ -z "$CROSS_COMPILE" ]; then
	echo "set CROSS_COMPILE env variable"
	exit 2
fi

$CROSS_COMPILE-gcc -v 2>/dev/null || exit

checksum() {
	local filename=$1
	[ -s "$filename" ] || return 1
	[ "`sha256sum -b \"$filename\" | cut -d' ' -f1`" = $SHA256SUM ] || return 1
	return 0
}

extract() {
	local fname="`basename "$PKGURL"`"

	# download if file is missing or corrupt
	if ! checksum "$fname"; then
		curl -L -o "$fname" "$PKGURL"
		if ! checksum "$fname"; then
			echo "download failed"
			exit 1
		fi
	fi

	# extract file
	rm -rf collectd-$PKGVER
	tar xvf "$fname"
}

prepare() {
	cd collectd-$PKGVER
}

build() {
	cd "${srcdir}/collectd-$PKGVER"

	./configure --host=$CROSS_COMPILE \
		--disable-werror \
		--prefix=/opt \
		--runstatedir=/var/run \
		--with-fp-layout=nothing --disable-pcie_errors

	rm -rf installroot
	mkdir  installroot
	make install DESTDIR=$PWD/installroot
}

package() {
	cd "${srcdir}/collectd-$PKGVER"

	# FIXME get rid of the .la files
	find installroot -iname '*.la' -exec rm {} \;

	# get rid of other files
	find installroot -type f -path '*/perl/*'      -exec rm -f {} \;
	find installroot -type f -path '*/pkgconfig/*' -exec rm -f {} \;
	find installroot -not -type d -iname 'libcollectdclient*' -exec rm -f {} \;
	find installroot -type f -path '*/bin/collectd*' -exec rm -f {} \;
	find installroot -type f -path '*/sbin/collectdmon' -exec rm -f {} \;
	find installroot -depth -empty -exec rmdir {} \;

	# add in our files
	install -m0644 -D $srcdir/files/collectd.control installroot/opt/lib/ipkg/info/collectd.control
	install -m0755 -D $srcdir/files/S99collectd      installroot/opt/etc/init.d/S99collectd

	# package files
	rm -f "$builddir/$OUT_TARBALL"
	fakeroot -- tar czvf "$builddir/$OUT_TARBALL" -C installroot/opt sbin lib etc share/collectd
}

srcdir=$PWD
[ ! -d "$srcdir" ] && mkdir "$srcdir"
cd "$srcdir"

# make build dir
builddir="$srcdir/build"
[ -d "$srcdir/build" ] || mkdir "$srcdir/build"

export CFLAGS="-DNEED_PRINTF"
export LDFLAGS="-s"

if [ "$#" -lt 1 ]; then
	extract
	prepare
	build
	package
else
	type $1 >/dev/null && $1 || echo "invalid build stage $1"
fi

