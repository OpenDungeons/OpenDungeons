# - Try to find CEGUI
# Once done, this will define
#
#  CEGUI_FOUND - system has CEGUI
#  CEGUI_INCLUDE_DIRS - the CEGUI include directories 
#  CEGUI_LIBRARIES - link these to use CEGUI

find_package(PkgConfig)
#pkg_check_modules(PC_CEGUI CEGUI)
#pkg_check_modules(PC_CEGUIOGRE CEGUI-OGRE)

message(STATUS "looking for headers")
#TODO - check for ogre renderer
#Look for headers
find_path(CEGUI_INCLUDE_DIR CEGUI.h
    HINTS $ENV{CEGUIDIR}
        ${PC_CEGUI_INCLUDEDIR}
    PATH_SUFFIXES include/CEGUI CEGUI include cegui/include
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
find_library(CEGUI_LIBRARY
  CEGUIBase
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

find_library(CEGUIOGRE_LIBRARY
 CEGUIOgreRenderer
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

set(CEGUI_LIBRARIES ${CEGUI_LIBRARY} ${CEGUIOGRE_LIBRARY})
set(CEGUI_INCLUDE_DIRS ${CEGUI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
#Set vars
find_package_handle_standard_args(CEGUI DEFAULT_MSG
                                    CEGUI_LIBRARY CEGUIOGRE_LIBRARY CEGUI_INCLUDE_DIR)
                                    
message(STATUS ${CEGUI_LIBRARIES})

