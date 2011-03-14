CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

#if( POLICY CMP0011 )
#  cmake_policy( SET CMP0011 OLD ) # or even better, NEW
#endif( POLICY CMP0011 )

PROJECT( OpenDungeons )
if ( WIN32 )

	SET( PROJECT_BINARY_NAME "OpenDungeons")
else()
	SET( PROJECT_BINARY_NAME "OpenDungeons.bin")
endif()
	
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 4 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 7 )

#INCLUDE(OpenAL)

SET( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules )

SET( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )


# Declare external dependencies
SET( Required_Packages OIS OGRE OpenAL )

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/include )

IF( NOT WIN32 )
  # Add compiler warnings
  ADD_DEFINITIONS( "-Wall -Wunused -ansi -Wno-deprecated" )
ENDIF( )
IF( WIN32 )

        SET( PTHREADS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/dep/pthreads/include/" )
        INCLUDE_DIRECTORIES( ${PTHREADS_INCLUDE_DIR} )
        SET( PTHREADS_LIBRARY_DIRS "${CMAKE_SOURCE_DIR}/dep/pthreads/lib/" )
        LINK_DIRECTORIES( ${PTHREADS_LIBRARY_DIRS} )
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


        LINK_LIBRARIES( "pthreadVC2.lib" )
        #link to shlwapi to get appdata path
        LINK_LIBRARIES( "ws2_32.lib" shlwapi)
        MESSAGE( STATUS "Using win pthreads" )
    ENDIF()
    IF(MINGW)
        LINK_LIBRARIES( "libpthreadGC2.a" )
        LINK_LIBRARIES( "ws2_32" shlwapi )
    ENDIF( )
ENDIF()

ADD_SUBDIRECTORY( src )


# Copy the configuration files to the binary directory
#IF( WIN32 )
#  ADD_CUSTOM_COMMAND(
#    OUTPUT ${PROJECT_BINARY_DIR}/plugins.cfg
#    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/plugins.cfg.windows ${PROJECT_BINARY_DIR}/#plugins.cfg
#    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/plugins.cfg.windows
#    )
#ELSE( )
#  ADD_CUSTOM_COMMAND(
#    OUTPUT ${PROJECT_BINARY_DIR}/plugins.cfg
#    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/plugins.cfg.linux ${PROJECT_BINARY_DIR}/#plugins.cfg
#    MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/plugins.cfg.linux
#    )
#ENDIF( )
#ADD_CUSTOM_COMMAND(
#  OUTPUT ${PROJECT_BINARY_DIR}/resources.cfg
#  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/resources.cfg ${PROJECT_BINARY_DIR}
#  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/resources.cfg
#  )
#ADD_CUSTOM_COMMAND(
#  OUTPUT ${PROJECT_BINARY_DIR}/media.cfg
#  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/media.cfg ${PROJECT_BINARY_DIR}
#  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/media.cfg
#  )
#ADD_CUSTOM_TARGET(
#  copy-config ALL
#  DEPENDS ${PROJECT_BINARY_DIR}/plugins.cfg ${PROJECT_BINARY_DIR}/resources.cfg ${PROJECT_BINARY_DIR}/#media.cfg
#  )

## Copy the media files to the binary directory
#IF( WIN32 )
#  FILE( COPY ${PROJECT_SOURCE_DIR}/Media DESTINATION ${PROJECT_BINARY_DIR} )
#ELSE( )
#  ADD_CUSTOM_COMMAND(
#    OUTPUT ${PROJECT_BINARY_DIR}/Media
#    COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_SOURCE_DIR}/Media ${PROJECT_BINARY_DIR}/Media
#    )
#  ADD_CUSTOM_TARGET( copy-media ALL DEPENDS ${PROJECT_BINARY_DIR}/Media )
#ENDIF( )

set(CPACK_SOURCE_IGNORE_FILES "~$" "${CMAKE_SOURCE_DIR}.*/.svn/" "${CMAKE_SOURCE_DIR}/debian/")



#find_package(OGRE REQUIRED)


INCLUDE_DIRECTORIES(${OPENAL_INCLUDE_DIR})

#configuration files

#Make a copy for use in the build directory, and later one copy for installing.
set(OPENDUNGEONS_MEDIA_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources.cfg)

#configure settings files

if ( WIN32 )
	set(OPENDUNGEONS_OGRE_PLUGIN_DIR ".")
	set(OPENDUNGEONS_DATA_PATH ".")
	set(OPENDUNGEONS_DATA_DEST ".")
#TODO - check if this allways will be right
else ()
	#only use d3d renderer plugins if we are on windows
	set(OPENDUNGEONS_COMMENT_D3D "#" )
	set(OPENDUNGEONS_OGRE_PLUGIN_DIR ${OGRE_LIBRARY_DIR_REL}/OGRE)
	
	set(OPENDUNGEONS_DATA_DEST share/games/${PROJECT_NAME})
	set(OPENDUNGEONS_DATA_PATH ${CMAKE_INSTALL_PREFIX}/${OPENDUNGEONS_DATA_DEST})
endif ()
set(OPENDUNGEONS_MEDIA_DIR ".")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins_d.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins_d.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg)
if ( UNIX )
	set(OPENDUNGEONS_BINARY_PATH ${CMAKE_INSTALL_PREFIX}/games/${PROJECT_BINARY_NAME})
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/OpenDungeons.in ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons)
endif ()

set(OPENDUNGEONS_CONFIGFILES ${CMAKE_CURRENT_SOURCE_DIR}/plugins.cfg)

#Installation
if (WIN32) #TODO - test
#install media
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${OPENDUNGEONS_DATA_DEST} PATTERN ".svn" EXCLUDE)
#install plugins.cfg and media.cfg
	install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})
#install resources.cfg
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
else()
#Make sure we ignore .svn files
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${OPENDUNGEONS_DATA_DEST} PATTERN ".svn" EXCLUDE )
	install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons DESTINATION games PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

#Packaging
set(CPACK_PACKAGE_NAME opendungeons)
set(CPACK_PACKAGE_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL})
set(CPACK_PACKAGE_CONTACT superman)
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcegui (>= 0.7.1), libois-1.2.0, libsfml-audio1.6, libopenal1, libogremain-1.7.2")
include(CPack)
