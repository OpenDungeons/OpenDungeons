## Linux portable binary release

This tutorial is based on information collected from those two sources:

* http://freegamedev.net/wiki/Portable_binaries
* http://www.cmake.org/Wiki/CMake_RPATH_handling

There are several ways to provide portable binaries for Linux-based operating 
systems. Here we chose to bundle the shared libraries needed by OpenDungeons 
in the tree, using a relative library path (RPATH) set to $ORIGIN/lib (i.e. a 
./lib subdirectory next to the main binary).
Some core shared libraries must not be bundled however, such as glibc, gcc, 
GL, pthread, ld, etc.

An important factor when building portable binaries is that the system glibc 
must be at least as old as the one of the oldest distribution that we want to 
support. In other words, we can expect glibc to be forwards-compatible but not 
backwards-compatible, so a binary built against e.g. glibc 2.20 will probably 
not run on a system with glibc 2.17.

For OpenDungeons 0.4.9, we built the binaries under Mageia 3 with glibc 2.17. 
This was not the oldest release available at that time (Debian Wheezy had 
glibc 2.13), but it had the advantage of being old enough for most users and 
recent enough to build OD's dependencies without trouble. OD is also still in 
alpha, so we don't have to make sure that the portable binaries will run on 
any system now and in the coming 10 years.

### Preparing the build environment

You will first need to install the chosen "old" release, either on a real 
partition, in a virtual machine or in a chroot. To install a Mageia chroot 
using urpmi on a Mageia system, see: https://wiki.mageia.org/en/Chroot

Then, build and install those core OD dependencies:

* OGRE >= 1.9.0
* OIS >= 1.3
* SFML >= 2.1
* CEGUI >= 0.8.0 (make sure to build and install OGRE and OIS before building 
CEGUI)

Depending on the chosen target system, some additional dependencies might be 
needed to build the OD dependencies.
It might also be needed to build a recent version of boost locally if the 
system's boost is not compatible with OGRE. Boost 1.53.0 from Mageia 3 was 
sufficient for OD 0.4.9.

### Building the portable binary release

In short: run the script located in dist/portable:

 * ```build-portable-binary.sh```

Note that the scripts must be run from the root of the source directory (i.e. 
the directory that holds the source folder and various data folders). The 
first script can be run with arguments which are passed directly to the CMake 
call.

The following sections give explanations about the steps performed in the
script.

#### 1. Building OpenDungeons with a RPATH

This is done by setting the install RPATH to $ORIGIN/lib{32,64} (with the $
properly escaped in the CMake call) using the CMAKE_INSTALL_RPATH variable.
Since we  won't use ```make install``` to generate the portable archive, we
also tell CMake to use the install RPATH even for the output of the build
step (if not, it only writes the RPATH when installing the binary). In the
end, we initialise the OD build with:

```
cmake -DCMAKE_SKIP_BUILD_RPATH=FALSE \
      -DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE \
      -DCMAKE_INSTALL_RPATH="\$ORIGIN/lib$archsuffix"
```

Since we don't use the install step, we must also strip the binary from the 
debug symbols ourselves.

#### 2. Bundling the needed shared libraries

We can list the libraries needed by the OD binary using ```ldd```. The given 
list gives references formatted this way:

```
libOgreMain.so.1.9.0 => /lib64/libOgreMain.so.1.9.0 (0x00007f2d122b4000)
libboost_system.so.1.55.0 => /lib64/libboost_system.so.1.55.0 
(0x00007f2d120b0000)
```

Therefore we process the third word of these lines to get the path to the 
library, and copy it in the ./lib{32,64} directory that we set as the RPATH.

Before that, we remove all libraries that we do not want to bundle using sed. 
The NOBUNDLE string in the 2nd script lists those variables. It might have to 
be adapted if we notice that some core libraries at the system still get 
bundled, or if some non-bundled libraries would actually be necessary.

The script also handles bundling the OGRE and CEGUI plugins that are needed to 
run OD. This part might need editing if we start using different plugins in 
future releases.

#### 3. Creating the portable binary tarball

After step 2., the portable binary is ready, we just have to clean up the 
stuff that players do not need (source code, CMake scripts, build directory, 
etc.) and (optionally) put everything in a neat bzip-compressed tarball.

The tarball can be made for one arch (thus containing the arch in the dirname,
like "Linux32" or "Linux64") or for both arches (thus being simply named
"Linux").
