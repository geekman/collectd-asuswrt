#!/bin/sh
#
# cross-compiles collectd for ASUSWRT
# call this script from the collectd source dir
#

set -e 

PKGVER=5.9.0

OUT_TARBALL=collectd-$PKGVER.tar.gz
SRCDIR="`dirname \"$0\"`/files"

[ -x ./configure ] || exit

fakeroot -v >/dev/null 2>/dev/null || exit

if [ -z "$CROSS_COMPILE" ]; then
	echo "set CROSS_COMPILE env variable"
	exit 2
fi

$CROSS_COMPILE-gcc -v 2>/dev/null || exit

[ -e config.status ] || ./configure --host=$CROSS_COMPILE \
	--prefix=/opt \
	--runstatedir=/var/run \
	--with-fp-layout=nothing --disable-pcie_errors

rm -rf installroot
mkdir  installroot
make install DESTDIR=$PWD/installroot

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
install -m0644 -D $SRCDIR/collectd.control installroot/opt/lib/ipkg/info/collectd.control
install -m0755 -D $SRCDIR/S99collectd      installroot/opt/etc/init.d/S99collectd

# package files
rm -f "$OUT_TARBALL"
fakeroot -- tar czvf "$OUT_TARBALL" -C installroot/opt sbin lib etc share/collectd

