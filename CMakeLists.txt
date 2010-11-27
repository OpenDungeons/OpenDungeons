CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

if( POLICY CMP0011 )
  cmake_policy( SET CMP0011 OLD ) # or even better, NEW
endif( POLICY CMP0011 )

PROJECT( OpenDungeons )
SET( PROJECT_BINARY_NAME "MapEditor")
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 1 )

#INCLUDE(OpenAL)

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules )

SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )


# Declare external dependencies
SET( Required_Packages OIS OGRE CEGUI CEGUIOGRE OpenAL OgreOggSound )

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include )

# Copy the configuration files to the binary directory
IF( WIN32 )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${PROJECT_BINARY_DIR}/plugins.cfg
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/plugins.cfg.windows ${PROJECT_BINARY_DIR}/plugins.cfg
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/plugins.cfg.windows
    )
ELSE( )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${PROJECT_BINARY_DIR}/plugins.cfg
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/plugins.cfg.linux ${PROJECT_BINARY_DIR}/plugins.cfg
    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/plugins.cfg.linux
    )
ENDIF( )
ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_BINARY_DIR}/resources.cfg
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/resources.cfg ${PROJECT_BINARY_DIR}
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/resources.cfg
  )
ADD_CUSTOM_COMMAND(
  OUTPUT ${PROJECT_BINARY_DIR}/media.cfg
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/media.cfg ${PROJECT_BINARY_DIR}
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/media.cfg
  )
ADD_CUSTOM_TARGET(
  copy-config ALL
  DEPENDS ${PROJECT_BINARY_DIR}/plugins.cfg ${PROJECT_BINARY_DIR}/resources.cfg ${PROJECT_BINARY_DIR}/media.cfg
  )

# Copy the media files to the binary directory
IF( WIN32 )
  FILE( COPY ${PROJECT_SOURCE_DIR}/Media DESTINATION ${PROJECT_BINARY_DIR} )
ELSE( )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${PROJECT_BINARY_DIR}/Media
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/Media ${PROJECT_BINARY_DIR}/Media
    )
  ADD_CUSTOM_TARGET( copy-media ALL DEPENDS ${PROJECT_BINARY_DIR}/Media )
ENDIF( )

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
  SET( PTHREADS_LIBRARY_DIRS "${CMAKE_SOURCE_DIR}/dep/pthreads/" )
  LINK_DIRECTORIES( ${PTHREADS_LIBRARY_DIRS} )
  LINK_LIBRARIES( "pthreadVC1.lib" )
  LINK_LIBRARIES( "ws2_32.lib" )
  MESSAGE( STATUS "Using win pthreads" )
ENDIF( )

#Find boost
#Needs to be updated with use of new boost release
set(Boost_ADDITIONAL_VERSIONS 1.42.0 1.42 1.43.0 1.43 1.44.0 1.44 1.45.0 1.45) 
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS   OFF)
find_package( Boost COMPONENTS thread )
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	if ( WIN32 )
		link_directories(${Boost_LIBRARY_DIRS}) ##Needed because VS wants to autolink
	endif()
	link_libraries(${Boost_LIBRARIES})
endif()
#if(CEGUI_FOUND)
link_directories(${CEGUI_LIB_DIR})


INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})

#link_libraries(OgreOggSound boost_thread-mt)
#if( NOT WIN32 )
#	message(STATUS ${OGREOGGSOUND_LIBRARY})
#	link_libraries(${OGREOGGSOUND_LIBRARY})
#endif ()

ADD_SUBDIRECTORY( src )

