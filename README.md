collectd on ASUSWRT
=====================

This repo helps with cross-compiling collectd for ASUSWRT routers.


1. You need to setup the [ASUSWRT toolchains](https://github.com/RMerl/am-toolchains)

2. Use this `build.sh` script to download, build and package collectd:

   ```
   CROSS_COMPILE=arm-brcm-linux-uclibcgnueabi ./build.sh
   ```

   You can specify the toolchain to use with the `CROSS_COMPILE` environment variable.

