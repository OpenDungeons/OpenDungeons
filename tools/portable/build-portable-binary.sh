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

# We do not use the `arch` command because it seems to output the arch
# of the host instead of the arch of the chroot (at least in a Mageia 32bit chroot)
if [ `getconf LONG_BIT` = "64" ]
then
    archsuffix=64
    archname=x86_64
else
    archsuffix=32
    archname=i586
fi

libdir=lib$archsuffix
binname=opendungeons.$archname
release=0.5.0



echo "=== Building the portable binary with an \$ORIGIN/$libdir RPATH ==="
rm -rf build && mkdir build
cd build
# We will use the "build" binary (without "make install" step)
# so we add the install RPATH to the build binary directly
cmake .. -DCMAKE_SKIP_BUILD_RPATH=FALSE \
         -DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE \
         -DCMAKE_INSTALL_RPATH="\$ORIGIN/$libdir" \
         $@

NCPUS_MAX=`/usr/bin/getconf _NPROCESSORS_ONLN`
make -j$NCPUS_MAX
cd ..



echo "=== Copying the generated binary and config to the main folder ==="

rm -rf $libdir && mkdir $libdir

cp -f build/opendungeons $binname
cp -f build/resources.cfg .
strip --strip-debug ./$binname

echo "=== Bundling the needed shared libraries in the ./$libdir folder ==="

# List the needed libraries
ldd ./$binname &> libraries.txt

# Remove the libraries we don't want to bundle
NOBUNDLE="linux-vdso linux-gate libpthread libstdc++ libc libm libgcc_s \
          libX11 libXt libXext libXmu libXpm libXau libXdmcp libSM libICE \
          libuuid libz libxcb libdl librt ld-linux ld-linux-x86-64"

for lib in $NOBUNDLE
do
    sed -i -e '/'"$lib"'.so/d' libraries.txt
done

# Parse the remaining libraries and copy them to ./$libdir
# If the linked lib is a symlink, copy also the target of the symlink
while read line
do
    # The path to the lib is the 3rd word
    libpath=$(echo $line | cut -d" " -f3)
    cp -f $libpath $libdir
done < libraries.txt

# Bundle OGRE plugins
plugindir=$(pkg-config OGRE --variable=plugindir)
cp -f $plugindir/{RenderSystem_GL.so.1.9.0,Plugin_OctreeSceneManager.so.1.9.0} $libdir

cat << EOF > plugins.cfg
# Defines plugins to load
Plugin=RenderSystem_GL

#Scene manager
Plugin=Plugin_OctreeSceneManager
EOF

# Bundle CEGUI plugins
moduledir=$(pkg-config CEGUI-0 --variable=moduledir)
cp -f $moduledir/{libCEGUIExpatParser.so,libCEGUICoreWindowRendererSet.so} $libdir



while true; do
    read -p "Do you wish to generate a single-arch (s) or dual-arch (d) tarball, or none (n)?" sdn
    case $sdn in
        [sSyY]* ) dirsuffix=$archsuffix; break;;
        [dD]* ) dirsuffix=""; break;;
        [Nn]* ) exit 0;;
        * ) echo "Please answer with single-arch (s), dual-arch (d) or none (n).";;
    esac
done

echo "=== Generating the portable binary release tarball ==="
dirname=OpenDungeons-$release-Linux$dirsuffix

rm -rf ../$dirname ../$dirname.tar.bz2
mkdir ../$dirname
cp -a * ../$dirname

cd ../$dirname
rm -rf build libraries.txt
rm -rf cmake dependencies dist source tools \
       CMakeLists.txt
cd ..

tar cvjf $dirname.tar.bz2 $dirname
rm -rf $dirname
