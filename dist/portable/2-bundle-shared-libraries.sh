#!/bin/sh
# Bundle the system's shared libraries in the ./lib folder
# This is meant to be used with the build-portable-binary.sh script

echo "=== Copying the generated binary and config to the main folder ==="

rm -rf lib && mkdir lib

cp -f build/opendungeons .
cp -f build/{resources.cfg,plugins.cfg} .
strip --strip-debug ./opendungeons

echo "=== Bundling the needed shared libraries in the ./lib folder ==="

# List the needed libraries
ldd ./opendungeons &> libraries.txt

# Remove the libraries we don't want to bundle
NOBUNDLE="linux-vdso linux-gate libpthread libstdc++ libc libm libgcc_s \
          libX11 libXt libXext libXmu libXpm libXau libXdmcp libSM libICE \
          libuuid libz libxcb libdl librt ld-linux ld-linux-x86-64"

for lib in $NOBUNDLE
do
    sed -i -e '/'"$lib"'.so/d' libraries.txt
done

# Parse the remaining libraries and copy them to ./lib
# If the linked lib is a symlink, copy also the target of the symlink
while read line
do
    # The path to the lib is the 3rd word
    libpath=$(echo $line | cut -d" " -f3)
    cp -f $libpath lib
done < libraries.txt

# Bundle OGRE plugins
plugindir=$(pkg-config OGRE --variable=plugindir)
cp -f $plugindir/{RenderSystem_GL.so.1.9.0,Plugin_OctreeSceneManager.so.1.9.0} lib

sed -i 's/PluginFolder=.*/PluginFolder=lib/' plugins.cfg

# Bundle CEGUI plugins
moduledir=$(pkg-config CEGUI-0 --variable=moduledir)
cp -f $moduledir/{libCEGUIExpatParser.so,libCEGUICoreWindowRendererSet.so} lib
