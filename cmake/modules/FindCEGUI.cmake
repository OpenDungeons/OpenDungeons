# - Try to find CEGUI
# Once done, this will define
#
#  CEGUI_FOUND - system has CEGUI
#  CEGUI_INCLUDE_DIRS - the CEGUI include directories 
#  CEGUI_LIBRARIES_RELEASE - link these to use CEGUI release
#  CEGUI_LIBRARIES_DEBUG - link these to use CEGUI debug

find_package(PkgConfig)
#pkg_check_modules(PC_CEGUI CEGUI)
#pkg_check_modules(PC_CEGUIOGRE CEGUI-OGRE)

message(STATUS "looking for headers")
#TODO - check for ogre renderer
#Look for headers
find_path(CEGUI_INCLUDE_DIR CEGUI/CEGUI.h
    HINTS $ENV{CEGUIDIR}
    ${PC_CEGUI_INCLUDEDIR}
    PATH_SUFFIXES include cegui/include cegui-0 include/cegui-0
    PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include/
    /usr/include/
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    )
    
#Look for libs
find_library(CEGUI_LIBRARY_REL
  CEGUIBase CEGUIBase-0
  HINTS
  $ENV{CEGUIDIR}
  ${PC_CEGUI_LIBDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
  /usr
  /usr/local
)

find_library(CEGUI_LIBRARY_DBG
  CEGUIBase_d CEGUIBase-0_d
  HINTS
  $ENV{CEGUIDIR}
  ${PC_CEGUI_LIBDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
  /usr
  /usr/local
)

find_library(CEGUI_OGRE_LIBRARY_REL
  CEGUIOgreRenderer CEGUIOgreRenderer-0
  HINTS
  $ENV{CEGUIDIR}
  ${PC_CEGUI_LIBDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
  /usr
  /usr/local
)

find_library(CEGUI_OGRE_LIBRARY_DBG
  CEGUIOgreRenderer_d CEGUIOgreRenderer-0_d
  HINTS
  $ENV{CEGUIDIR}
  ${PC_CEGUI_LIBDIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  /sw
  /opt/local
  /opt/csw
  /opt
  /usr
  /usr/local
)

# if only one debug/release variant is found, set the other to be equal to the found one
if(CEGUI_LIBRARY_REL AND CEGUI_LIBRARY_DBG)
set(CEGUI_LIBRARY_RELEASE ${CEGUI_LIBRARY_REL})
set(CEGUI_LIBRARY_DEBUG ${CEGUI_LIBRARY_DBG})
endif()
if(CEGUI_LIBRARY_REL AND NOT CEGUI_LIBRARY_DBG)
set(CEGUI_LIBRARY_RELEASE ${CEGUI_LIBRARY_REL})
set(CEGUI_LIBRARY_DEBUG ${CEGUI_LIBRARY_REL})
endif()
if(CEGUI_LIBRARY_DBG AND NOT CEGUI_LIBRARY_REL)
set(CEGUI_LIBRARY_RELEASE ${CEGUI_LIBRARY_DBG})
set(CEGUI_LIBRARY_DEBUG ${CEGUI_LIBRARY_DBG})
endif()

if(CEGUI_OGRE_LIBRARY_REL AND CEGUI_OGRE_LIBRARY_DBG)
set(CEGUI_OGRE_LIBRARY_RELEASE ${CEGUI_OGRE_LIBRARY_REL})
set(CEGUI_OGRE_LIBRARY_DEBUG ${CEGUI_OGRE_LIBRARY_DBG})
endif()
if(CEGUI_OGRE_LIBRARY_REL AND NOT CEGUI_OGRE_LIBRARY_DBG)
set(CEGUI_OGRE_LIBRARY_RELEASE ${CEGUI_OGRE_LIBRARY_REL})
set(CEGUI_OGRE_LIBRARY_DEBUG ${CEGUI_OGRE_LIBRARY_REL})
endif()
if(CEGUI_OGRE_LIBRARY_DBG AND NOT CEGUI_OGRE_LIBRARY_REL)
set(CEGUI_OGRE_LIBRARY_RELEASE ${CEGUI_OGRE_LIBRARY_DBG})
set(CEGUI_OGRE_LIBRARY_DEBUG ${CEGUI_OGRE_LIBRARY_DBG})
endif()


set(CEGUI_LIBRARIES_RELEASE optimized ${CEGUI_LIBRARY_RELEASE}
   optimized ${CEGUI_OGRE_LIBRARY_RELEASE})
set(CEGUI_LIBRARIES_DEBUG debug ${CEGUI_LIBRARY_DEBUG}
   debug ${CEGUI_OGRE_LIBRARY_DEBUG})

set(CEGUI_INCLUDE_DIRS ${CEGUI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
#Set vars
find_package_handle_standard_args(CEGUI DEFAULT_MSG
                                    CEGUI_LIBRARY_RELEASE CEGUI_OGRE_LIBRARY_RELEASE CEGUI_INCLUDE_DIR)
                                    

