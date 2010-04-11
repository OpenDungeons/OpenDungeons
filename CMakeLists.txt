CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

if( POLICY CMP0011 )
  cmake_policy( SET CMP0011 OLD ) # or even better, NEW
endif( POLICY CMP0011 )

PROJECT( OpenDungeons )
SET( PROJECT_BINARY_NAME "MapEditor")
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 1 )

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules )

SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )

# Declare external dependencies
SET( Required_Packages OIS OGRE CEGUI CEGUIOGRE )

# searches for all .cpp and .h files in the current directory and add them 
# to the current project
FILE(GLOB folder_source ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
SOURCE_GROUP(${PROJECT_BINARY_NAME} FILES ${folder_source})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

IF( NOT WIN32 )
  # Add compiler warnings
  ADD_DEFINITIONS( "-Wall -Wunused -ansi -Wno-deprecated" )
ENDIF( )

IF( MSVC_IDE )
  IF( EXISTS $ENV{OGRE_HOME} )
    SET( OGRESDK $ENV{OGRE_HOME} )
    STRING( REGEX REPLACE "[\\]" "/" OGRESDK "${OGRESDK}" )
    MESSAGE( STATUS "Adding Additional Link Directories" )
    LINK_DIRECTORIES( ${OGRESDK}/lib )
  ENDIF( )

  SET( PLATFORM_C_FLAGS "/W3 /MD /Od /D \"WIN32\" /D \"_WINDOWS\" /Gm /Gy /fp:fast /ZI /EHsc /SUBSYSTEM:WINDOWS /ENTRY:\"main\"" )
  SET( PLATFORM_C_FLAGS_DEBUG "/W3 /MDd /Od /Gm /Gy /fp:fast /ZI" )

  SET( CMAKE_CXX_FLAGS "${PLATFORM_C_FLAGS}" )
  SET( CMAKE_CXX_FLAGS_RELEASE "${PLATFORM_C_FLAGS}" )
  SET( CMAKE_CXX_FLAGS_DEBUG "${PLATFORM_C_FLAGS_DEBUG}" )

  SET( PTHREADS_INCLUDE_DIR "dep/pthreads/" )
  INCLUDE_DIRECTORIES( ${PTHREADS_INCLUDE_DIR} )
  SET( PTHREADS_LIBRARY_DIRS "dep/pthreads/" )
  LINK_DIRECTORIES( ${PTHREADS_LIBRARY_DIRS} )
  LINK_LIBRARIES( "pthreadVC1.lib" )
  LINK_LIBRARIES( "ws2_32.lib" )
  MESSAGE( STATUS "Using win pthreads" )
ENDIF( )

# Create the binary file
ADD_EXECUTABLE( ${PROJECT_BINARY_NAME} WIN32 ${folder_source} )

# This foreach loads all of the packages specified as required.
FOREACH( Package ${Required_Packages} )
  FIND_PACKAGE( ${Package} REQUIRED )
  IF( ${Package}_FOUND )
    INCLUDE_DIRECTORIES( ${${Package}_INCLUDE_DIR} )
    IF( ${Package}_LIBRARIES )
      TARGET_LINK_LIBRARIES( ${PROJECT_BINARY_NAME} ${${Package}_LIBRARIES} )
    ELSE( )
      LINK_DIRECTORIES( ${${Package}_LIBRARY_DIRS} )
    ENDIF( )
  ELSE( )
    MESSAGE( STATUS "${Package} not found" )
  ENDIF( )
ENDFOREACH( )
