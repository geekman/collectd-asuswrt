#!/bin/sh
#
# cross-compiles collectd for ASUSWRT
# call this script from the collectd source dir
#

set -e 

CROSS_COMPILE=arm-brcm-linux-uclibcgnueabi
SRCDIR="`dirname \"$0\"`"

[ -x ./configure ] || exit

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

rm -f collectd.tar.gz
tar czvf collectd.tar.gz -C installroot/opt sbin lib etc share/collectd

