#!/bin/sh
#git clone git://git.gnome.org/libxml2
#cd libxml2
make clean
CC=arm-linux-androideabi-gcc CXX=arm-linux-androideabi-g++ ./autogen.sh --host=arm-linux-androideabi --prefix="/home/cpasjuste/dev/android/sdk/toolchain/sysroot/usr" --enable-static --disable-shared
rpl "testapi\$(EXEEXT) testModule\$(EXEEXT) runtest\$(EXEEXT)" "testapi\$(EXEEXT) testModule\$(EXEEXT)" Makefile
rpl "runxmlconf\$(EXEEXT) testrecurse\$(EXEEXT)" "runxmlconf\$(EXEEXT)" Makefile
make && make install
#cd ..

