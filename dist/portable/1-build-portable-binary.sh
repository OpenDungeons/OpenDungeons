#!/bin/sh
# Build a portable binary for OpenDungeons, i.e. using a relative library path
# (RPATH) to link it to shared libraries in $ORIGIN/lib
#
# Based on:
# http://freegamedev.net/wiki/Portable_binaries
# http://www.cmake.org/Wiki/CMake_RPATH_handling

# Error out if we are not at the root of the source directory
if [ ! -d ./source ]
then
    echo -e "The ./source directory seems not to be present.\nYou need to run this script from the root of the OpenDungeons source directory."
    exit 1
fi

echo "=== Building the portable binary with an \$ORIGIN/lib RPATH ==="
rm -rf build && mkdir build
pushd build
# We will use the "build" binary (without "make install" step)
# so we add the install RPATH to the build binary directly
cmake .. -DCMAKE_SKIP_BUILD_RPATH=FALSE \
         -DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE \
         -DCMAKE_INSTALL_RPATH="\$ORIGIN/lib" \
         $@

NCPUS_MAX=`/usr/bin/getconf _NPROCESSORS_ONLN`
make -j$NCPUS_MAX
popd

cp -f build/opendungeons .
cp -f build/{resources.cfg,plugins.cfg} .
strip --strip-debug ./opendungeons
