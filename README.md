collectd on ASUSWRT
=====================

This repo helps with cross-compiling collectd for ASUSWRT routers.


1. You need to setup the [ASUSWRT toolchains](https://github.com/RMerl/am-toolchains)

2. Use this `build.sh` script to download, build and package collectd:

   ```
   CROSS_COMPILE=arm-brcm-linux-uclibcgnueabi ./build.sh
   ```

   You can specify the toolchain to use with the `CROSS_COMPILE` environment variable.


Installation
--------------

You should have a writable partition mounted at `/opt`.
This is usually a JFFS partition on flash, or an external USB drive that is
plugged into the router.

To install collectd, download the tar.gz file onto your router (most likely to
`/tmp`), then unpack into `/opt`:

```
$ cd /opt
$ tar xzvf /tmp/collectd-$ver.tar.gz
```

You need to edit the `collectd.conf` configuration file.

The collectd service should start automatically on boot.


brcm_wl module
================

The inclusion of the `brcm_wl` plugin enables collection of detailed statistics
of wireless clients, such as association time, idle time, TX/RX bytes and
packets, RSSI levels, etc.

This plugin only works on (certain?) Broadcom wireless chipsets, because it
communicates directly with the driver to retrieve the information.

I have tested it to work on the *ASUS RT-AC68U* and the *D-Link DIR-868L*.

