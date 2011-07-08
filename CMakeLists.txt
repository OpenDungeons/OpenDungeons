CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

#if( POLICY CMP0011 )
#  cmake_policy( SET CMP0011 OLD ) # or even better, NEW
#endif( POLICY CMP0011 )

##################################
#### Project settings ############
##################################
PROJECT( OpenDungeons )

#postfix bin if we are on linux as we want to launch with a script
if ( WIN32 )
	set( PROJECT_BINARY_NAME "OpenDungeons")
else()
	set( PROJECT_BINARY_NAME "OpenDungeons.bin")
endif()
	
# Project version
set( ${PROJECT_NAME}_MAJOR_VERSION 0 )
set( ${PROJECT_NAME}_MINOR_VERSION 4 )
set( ${PROJECT_NAME}_PATCH_LEVEL 8 )

# Where to look for modules
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules )

# Where we want the binary
set( EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR} )

##################################
#### Find packages ###############
##################################

find_package(OIS REQUIRED)
find_package(OGRE REQUIRED)
find_package(CEGUI REQUIRED)
find_package(SFML REQUIRED COMPONENTS Audio System)


# boost uses some extra settings
#NOTE: Versions var has to be updated with use of new boost release 
# to find the boost release.
set(Boost_ADDITIONAL_VERSIONS 1.42.0 1.42 1.43.0 1.43 1.44.0 1.44 1.45.0 1.45)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS   OFF)
if(WIN32 AND ${OGRE_FOUND})
	#On windows we can try to use boost from the Ogre SDK instead
    #the 2010 package has a different boost version for some reason
    set( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${ENV_OGRE_HOME}/boost_1_42/lib 
                ${ENV_OGRE_HOME}/boost_1_44/lib)
    set( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${ENV_OGRE_HOME}/boost_1_42 
                ${ENV_OGRE_HOME}/boost_1_44)
    #We set these instead of searching if using MSVC
    #This is because the libs in the ogre dir has a lib prefix
    #making findboost not find them
	set( OD_BOOST_LIB_DIRS  ${ENV_OGRE_HOME}/boost_1_44/lib ${ENV_OGRE_HOME}/boost_1_42/lib)
	set( OD_BOOST_INCLUDE_DIRS ${ENV_OGRE_HOME}/boost_1_44 ${ENV_OGRE_HOME}/boost_1_42)

endif()
if(MINGW)
    find_package( Boost COMPONENTS thread REQUIRED)
endif()



#OpenAL
#If we are on windows we can also look for OpenAL where SFML is
if(WIN32 AND ${SFML_FOUND})
    set( CMAKE_LIBRARY_PATH ${SFMLDIR}/extlibs/libs-vc2005 ${SFMLDIR}/extlibs/libs-mingw)
    set( CMAKE_INCLUDE_PATH ${SFMLDIR}/extlibs/headers ${SFMLDIR}/extlibs/headers/AL)
endif()
find_package(OpenAL REQUIRED)

##################################
#### Headers and linking #########
##################################

include_directories( ${CMAKE_SOURCE_DIR}/include )
include_directories( ${CMAKE_SOURCE_DIR}/include/lua )
include_directories( ${CMAKE_SOURCE_DIR}/include/tinygettext )

#Provide dirent.h if visual studio is used
if(MSVC)
    include_directories( ${CMAKE_SOURCE_DIR}/include/dirent )
endif()


#Linking is set up in src/CMakeLists.txt
ADD_SUBDIRECTORY( src )

##################################
#### Configure settings files ####
##################################

#Make a copy for use in the build directory, and later one copy for installing.
#NOTE - this var is only used in the config file
#set(OPENDUNGEONS_MEDIA_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OPENDUNGEONS_MEDIA_DIR ".")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources.cfg)

if ( WIN32 )
#On windows, use current directory for plugins and data for now
	set(OPENDUNGEONS_OGRE_PLUGIN_DIR ".")
	set(OPENDUNGEONS_DATA_PATH ".")
	set(OPENDUNGEONS_DATA_DEST ".")

else ()

	#only use d3d renderer plugins if we are on windows
	set(OPENDUNGEONS_COMMENT_D3D "#" )
	
	#On linux, we should look in the standard paths for plugins
	set(OPENDUNGEONS_OGRE_PLUGIN_DIR ${OGRE_LIBRARY_DIR_REL}/OGRE)
	
	#Set data location to where we install if we want to use the installer
	set(OPENDUNGEONS_DATA_DEST share/games/${PROJECT_NAME})
	set(OPENDUNGEONS_DATA_PATH ${CMAKE_INSTALL_PREFIX}/${OPENDUNGEONS_DATA_DEST})
endif ()

#Do the configuration
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins_d.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins_d.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg)
if ( UNIX )
    #On linux, set up the launcher script
    #TODO - add key repeat thing
	set(OPENDUNGEONS_BINARY_PATH ${CMAKE_INSTALL_PREFIX}/games/${PROJECT_BINARY_NAME})
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/OpenDungeons.in ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons)
endif ()

##################################
#### Installation ################
##################################

#TODO - fix this with the new media paths

set(OPENDUNGEONS_CONFIGFILES ${CMAKE_CURRENT_SOURCE_DIR}/plugins.cfg ${CMAKE_CURRENT_SOURCE_DIR}/media.cfg)

if (WIN32) #NOTE - not really used on windows
#install media, Make sure we ignore .svn files
#	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Media DESTINATION ${OPENDUNGEONS_DATA_DEST} PATTERN ".svn" EXCLUDE)
#install plugins.cfg and media.cfg
	install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})
#install resources.cfg
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
else()

#	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Media DESTINATION ${OPENDUNGEONS_DATA_DEST} PATTERN ".svn" EXCLUDE )
	install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons DESTINATION games PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

##################################
#### Copy files to binary dir ####
##################################

# Copy level dir from git to build dir if this is not an in-source build
if( NOT ${CMAKE_CURRENT_SOURCE_DIR} EQUAL ${CMAKE_CURRENT_BINARY_DIR})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/levels_git/Test.level 
    ${CMAKE_CURRENT_BINARY_DIR}/levels_git/Test.level)
endif()

##################################
#### Packaging ###################
##################################

#Not used at the moment, more flexible to do the deb packages manually
#TODO - set up for making tarballs
set(CPACK_SOURCE_IGNORE_FILES "~$" "${CMAKE_SOURCE_DIR}.*/.svn/" "${CMAKE_SOURCE_DIR}/debian/")
set(CPACK_PACKAGE_NAME opendungeons)
set(CPACK_PACKAGE_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL})
set(CPACK_PACKAGE_CONTACT opendungeons team)
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcegui (>= 0.7.1), libois-1.2.0, libsfml-audio1.6, libopenal1, libogremain-1.7.2")
include(CPack)
