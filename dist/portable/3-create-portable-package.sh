#!/bin/sh
# Remove all development content to prepare a portable binary release
# containing only the data, binary and bundled libs

echo "=== Generating the portable binary release tarball ==="

# We do not use the `arch` command because it seems to output the arch
# of the host instead of the arch of the chroot (at least in a Mageia 32bit chroot)
if [ `getconf LONG_BIT` = "64" ]
then
    arch="x86_64"
else
    arch="i586"
fi

dirname="opendungeons-0.4.9-linux-$arch"
rm -rf ../$dirname ../$dirname.tar.xz
mkdir ../$dirname
cp -a * ../$dirname
cd ..

pushd $dirname
rm -rf build libraries.txt
rm -rf cmake dependencies dist source \
       CMakeLists.txt od.cppcheck README

mv opendungeons opendungeons.$arch
popd

tar cvJf $dirname.tar.xz $dirname
rm -rf $dirname
