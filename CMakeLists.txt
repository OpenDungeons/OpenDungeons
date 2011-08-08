cmake_minimum_required(VERSION 2.8)

##################################
#### Useful variables ############
##################################

set(SRC "${CMAKE_SOURCE_DIR}/src")
set(ANGELSCRIPT_DIR "${CMAKE_SOURCE_DIR}/dep/angelscript")
set(TINYGETTEXT_DIR "${SRC}/tinygettext")
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMake build type is not set, defaulting to 'Release'")
    set(CMAKE_BUILD_TYPE "Release")
endif()

##################################
#### add external cmake projects #
##################################

#Build AngelScript from its own CMakeLists.txt
#add_subdirectory(${ANGELSCRIPT_DIR}/angelscript/projects/cmake)

##################################
#### Source files (.cpp) #########
##################################

set(OD_SOURCEFILES
     #AngelScript Addon sources
     ${ANGELSCRIPT_DIR}/add_on/scripthelper/scripthelper.cpp
     ${ANGELSCRIPT_DIR}/add_on/scriptstdstring/scriptstdstring.cpp
     
     #Tinygettext sources
     ${TINYGETTEXT_DIR}/dictionary_manager.cpp
     ${TINYGETTEXT_DIR}/dictionary.cpp
     ${TINYGETTEXT_DIR}/language.cpp
     ${TINYGETTEXT_DIR}/plural_forms.cpp
     ${TINYGETTEXT_DIR}/po_parser.cpp
     ${TINYGETTEXT_DIR}/tinygettext.cpp
     
     #OpenDungeons sources
     ${SRC}/ActiveObject.cpp
     ${SRC}/AttackableObject.cpp
     ${SRC}/AnimatedObject.cpp
     ${SRC}/ASWrapper.cpp
     ${SRC}/CameraManager.cpp
     ${SRC}/ChatMessage.cpp
     ${SRC}/Client.cpp
     ${SRC}/ClientNotification.cpp
     ${SRC}/Console.cpp
     ${SRC}/CreatureAction.cpp
     ${SRC}/CreatureClass.cpp
     ${SRC}/Creature.cpp
     ${SRC}/CreatureSound.cpp
     ${SRC}/DirectionalTrap.cpp
     ${SRC}/Field.cpp
     ${SRC}/Functions.cpp
     ${SRC}/GameMap.cpp
     ${SRC}/GameState.cpp
     ${SRC}/GoalClaimNTiles.cpp
     ${SRC}/Goal.cpp
     ${SRC}/GoalKillAllEnemies.cpp
     ${SRC}/GoalMineNGold.cpp
     ${SRC}/GoalProtectCreature.cpp
     ${SRC}/GoalProtectDungeonTemple.cpp
     ${SRC}/Gui.cpp
     ${SRC}/InputManager.cpp
     ${SRC}/LogManager.cpp
     ${SRC}/main.cpp
     ${SRC}/MapLight.cpp
     ${SRC}/MiniMap.cpp
     ${SRC}/MissileObject.cpp
     ${SRC}/MusicPlayer.cpp
     ${SRC}/ODApplication.cpp
     ${SRC}/ODFrameListener.cpp
     ${SRC}/Player.cpp
     ${SRC}/ProximityTrap.cpp
     ${SRC}/RadialVector2.cpp
     ${SRC}/Random.cpp
     ${SRC}/RenderManager.cpp
     ${SRC}/RenderRequest.cpp
     ${SRC}/ResourceManager.cpp
     ${SRC}/Room.cpp
     ${SRC}/RoomDojo.cpp
     ${SRC}/RoomDungeonTemple.cpp
     ${SRC}/RoomForge.cpp
     ${SRC}/RoomObject.cpp
     ${SRC}/RoomPortal.cpp
     ${SRC}/RoomQuarters.cpp
     ${SRC}/RoomTreasury.cpp
     ${SRC}/Seat.cpp
     ${SRC}/Server.cpp
     ${SRC}/ServerNotification.cpp
     ${SRC}/Socket.cpp
     ${SRC}/SoundEffectsHelper.cpp
     ${SRC}/Spell.cpp
     ${SRC}/TextRenderer.cpp
     ${SRC}/Tile.cpp
     ${SRC}/TileCoordinateMap.cpp
     ${SRC}/Translation.cpp
     ${SRC}/Trap.cpp
     ${SRC}/TrapBoulder.cpp
     ${SRC}/TrapCannon.cpp
     ${SRC}/Weapon.cpp
)

##################################
#### Project settings ############
##################################

project(OpenDungeons)

#postfix bin if we are on linux as we want to launch with a script
if(WIN32)
	set( PROJECT_BINARY_NAME "OpenDungeons")
else()
	set( PROJECT_BINARY_NAME "OpenDungeons.bin")
endif()
	
# Project version
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 4)
set(${PROJECT_NAME}_PATCH_LEVEL   8)

# Where to look for modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

# Where we want the binary
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

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
    find_package(Boost COMPONENTS thread REQUIRED)
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

#add all project specific include directorieS
include_directories(
    #OpenDungeons includes
    ${CMAKE_SOURCE_DIR}/include

    #tinygettext includes
    ${CMAKE_SOURCE_DIR}/include/tinygettext

    #Angelscript includes    
    ${ANGELSCRIPT_DIR}/angelscript/include/

    #AngelScript Addons, in-comment only the ones we actually use
#    ${ANGELSCRIPT_DIR}/add_on/autowrapper
#    ${ANGELSCRIPT_DIR}/add_on/clib
#    ${ANGELSCRIPT_DIR}/add_on/contextmgr
#    ${ANGELSCRIPT_DIR}/add_on/debugger
#    ${ANGELSCRIPT_DIR}/add_on/scriptany
#    ${ANGELSCRIPT_DIR}/add_on/scriptarray
#    ${ANGELSCRIPT_DIR}/add_on/scriptbuilder
#    ${ANGELSCRIPT_DIR}/add_on/scriptdictionary
#    ${ANGELSCRIPT_DIR}/add_on/scriptfile
    ${ANGELSCRIPT_DIR}/add_on/scripthelper
#    ${ANGELSCRIPT_DIR}/add_on/scriptmath
#    ${ANGELSCRIPT_DIR}/add_on/scriptmath3d
    ${ANGELSCRIPT_DIR}/add_on/scriptstdstring

    # FIXME This should be handled by the find script
    #external packages includes
    ${CEGUI_INCLUDE_DIR}
    ${SFML_INCLUDE_DIR}
    ${OGRE_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${OIS_INCLUDE_DIRS}
)

link_directories(${ANGELSCRIPT_DIR}/angelscript/lib)

if(WIN32)
    if(MINGW)
        # Boost
        link_libraries(${Boost_LIBRARIES})
        include_directories(${Boost_INCLUDE_DIRS})
    endif()
    
    if(MSVC)
        #Provide dirent.h if visual studio is used
        include_directories( ${CMAKE_SOURCE_DIR}/include/dirent )
    
        message(STATUS "MSVC: " ${OD_BOOST_INCLUDE_DIRS})
        message(STATUS "MSVC: " ${OD_BOOST_LIB_DIRS})
        include_directories(${OD_BOOST_INCLUDE_DIRS})
        link_directories(${OD_BOOST_LIB_DIRS})
    endif()

    #Pthreads on windows
    include_directories(${CMAKE_SOURCE_DIR}/dep/pthreads/include/)
    link_directories(${CMAKE_SOURCE_DIR}/dep/pthreads/lib/)
endif()

##################################
#### Binary ######################
##################################

# Create the binary file (WIN32 makes sure there is no console window on windows.)
add_executable(${PROJECT_BINARY_NAME} WIN32 ${OD_SOURCEFILES})

##################################
#### Install targets #############
##################################

# Add install target
if(WIN32)
    install(TARGETS ${PROJECT_BINARY_NAME}
        DESTINATION .
    ) 
else ()
    install(TARGETS ${PROJECT_BINARY_NAME}
        DESTINATION games
    )
endif() 

##################################
#### Link libraries ##############
##################################

# Link libraries
target_link_libraries(
    #target
    ${PROJECT_BINARY_NAME}

    #libraries
    ${OGRE_LIBRARIES} 
    ${OGRE_RTShaderSystem_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${OIS_LIBRARIES}
    ${CEGUI_LIBRARIES}
    Angelscript
)
    
# MSVC automatically links boost
if(NOT MSVC)
    target_link_libraries(${PROJECT_BINARY_NAME} ${Boost_LIBRARIES})
endif()

# Link sfml
# Use debug libraries if they exist
if(${SFML_AUDIO_LIBRARY_SHARED_DEBUG})
    target_link_libraries(
        ${PROJECT_BINARY_NAME}

        optimized ${SFML_AUDIO_LIBRARY_SHARED_NONDEBUG}
        optimized ${SFML_SYSTEM_LIBRARY_SHARED_NONDEBUG} 
        debug ${SFML_AUDIO_LIBRARY_SHARED_DEBUG} 
        debug ${SFML_SYSTEM_LIBRARY_SHARED_DEBUG}
    ) 
else()
    target_link_libraries(${PROJECT_BINARY_NAME} 
        ${SFML_AUDIO_LIBRARY_SHARED_NONDEBUG}
        ${SFML_SYSTEM_LIBRARY_SHARED_NONDEBUG})
endif()

# Link pthreads and userenv if we are on windows
if(WIN32)
    if(MSVC_IDE)
        #Set some extra compiler flags
        #TODO - investigate if these are what they should be
        set( PLATFORM_C_FLAGS "/W3 /MD /Od /D \"WIN32\" /D \"_WINDOWS\" /Gm /Gy /fp:fast /ZI /EHsc /SUBSYSTEM:WINDOWS /ENTRY:\"main\"" )
        set( PLATFORM_C_FLAGS_DEBUG "/W3 /MDd /Od /Gm /Gy /fp:fast /ZI" )

        set( CMAKE_CXX_FLAGS "${PLATFORM_C_FLAGS}" )
        set( CMAKE_CXX_FLAGS_RELEASE "${PLATFORM_C_FLAGS}" )
        set( CMAKE_CXX_FLAGS_DEBUG "${PLATFORM_C_FLAGS_DEBUG}" )

        #Link to pthreads
        target_link_libraries(
            ${PROJECT_BINARY_NAME}

            "pthreadVC2.lib"
            "ws2_32.lib"
            Userenv
        )
        message( STATUS "Using win pthreads" )
    endif()

    if(MINGW)
        target_link_libraries(
            ${PROJECT_BINARY_NAME}

            "libpthreadGC2.a"
            "ws2_32"
            Userenv
        )
    endif()
else()
    #TODO - move compiler defs
    #Add compiler warnings
    add_definitions( "-Wall -Wunused -ansi -Wno-deprecated" )
endif()

##################################
#### Configure settings files ####
##################################

#Make a copy for use in the build directory, and later one copy for installing.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources.cfg
)

if(WIN32)
    #On windows, use current directory for plugins and data for now
    set(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ".")
    set(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG ".")
    set(OPENDUNGEONS_DATA_PATH ".")
    set(OPENDUNGEONS_DATA_DEST ".")
else()
    #only use d3d renderer plugins if we are on windows
    set(OPENDUNGEONS_COMMENT_D3D "#")
    #On linux, we should look in the standard paths for plugins
    if(${OGRE_PLUGIN_DIR_REL} STREQUAL NOTFOUND)
        #Ugly workaround for plugin dir not being set properly by the ogre find script.
        get_filename_component(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ${OGRE_RenderSystem_GL_LIBRARY_REL} PATH)
    else()
        set(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ${OGRE_PLUGIN_DIR_REL})
    endif()
    if(${OGRE_PLUGIN_DIR_DBG} STREQUAL NOTFOUND)
        get_filename_component(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG ${OGRE_RenderSystem_GL_LIBRARY_DBG} PATH)
    else()
        set(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG ${OGRE_PLUGIN_DIR_DBG})
    endif()

    message(STATUS "Plugin path rel: " ${OPENDUNGEONS_OGRE_PLUGIN_DIR_REL})
    message(STATUS "Plugin path dbg: " ${OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG})

    #Set data location to where we install if we want to use the installer
    set(OPENDUNGEONS_DATA_DEST share/games/${PROJECT_NAME})
    set(OPENDUNGEONS_DATA_PATH ${CMAKE_INSTALL_PREFIX}/${OPENDUNGEONS_DATA_DEST})
endif()

#Do the configuration
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/plugins_d.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins_d.cfg)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg)
if(UNIX)
    #On linux, set up the launcher script
    #TODO - add key repeat thing
	set(OPENDUNGEONS_BINARY_PATH ${CMAKE_INSTALL_PREFIX}/games/${PROJECT_BINARY_NAME})
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/OpenDungeons.in ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons)
endif()

##################################
#### Installation ################
##################################

set(OPENDUNGEONS_CONFIGFILES ${CMAKE_CURRENT_SOURCE_DIR}/plugins.cfg ${CMAKE_CURRENT_SOURCE_DIR}/media.cfg)

if (WIN32) #NOTE - not really used on windows
	install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})	
    #install resources.cfg
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
else()
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
