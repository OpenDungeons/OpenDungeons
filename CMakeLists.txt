cmake_minimum_required(VERSION 2.8)

##################################
#### Useful variables ############
##################################

#project paths
set(SRC "${CMAKE_SOURCE_DIR}/source")
set(DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/dependencies")
set(ANGELSCRIPT_DIR "${DEPENDENCIES_DIR}/angelscript")
set(ANGELSCRIPT_SRC_DIR "${ANGELSCRIPT_DIR}/angelscript/source")
set(ANGELSCRIPT_ADDON_DIR "${ANGELSCRIPT_DIR}/add_on")
set(TINYGETTEXT_DIR "${DEPENDENCIES_DIR}/tinygettext")
set(TINYXML_DIR "${DEPENDENCIES_DIR}/tinyxml2")

#cmake paths
set(CMAKE_CONFIG_DIR "${CMAKE_SOURCE_DIR}/cmake/config")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMake build type is not set, defaulting to 'Release'")
    set(CMAKE_BUILD_TYPE "Release")
endif()

##################################
#### Source files (.cpp) #########
##################################

#Add new .cpp files here so that they get compiled

set(OD_SOURCEFILES
    #AngelScript sources
	${ANGELSCRIPT_SRC_DIR}/as_atomic.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_builder.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_bytecode.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_arm.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_mips.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_ppc.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_ppc_64.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_sh4.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_x64_gcc.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_x64_msvc.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_x64_mingw.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_x86.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_callfunc_xenon.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_compiler.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_configgroup.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_context.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_datatype.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_gc.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_generic.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_globalproperty.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_memory.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_module.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_objecttype.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_outputbuffer.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_parser.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_restore.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_scriptcode.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_scriptengine.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_scriptfunction.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_scriptnode.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_scriptobject.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_string.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_string_util.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_thread.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_tokenizer.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_typeinfo.cpp
    ${ANGELSCRIPT_SRC_DIR}/as_variablescope.cpp

    #AngelScript Addon sources
    ${ANGELSCRIPT_ADDON_DIR}/scriptarray/scriptarray.cpp
    ${ANGELSCRIPT_ADDON_DIR}/scriptbuilder/scriptbuilder.cpp
    ${ANGELSCRIPT_ADDON_DIR}/scripthelper/scripthelper.cpp
    ${ANGELSCRIPT_ADDON_DIR}/scriptstdstring/scriptstdstring.cpp
    
    #Tinygettext sources
    ${TINYGETTEXT_DIR}/dictionary_manager.cpp
    ${TINYGETTEXT_DIR}/dictionary.cpp
    ${TINYGETTEXT_DIR}/language.cpp
    ${TINYGETTEXT_DIR}/plural_forms.cpp
    ${TINYGETTEXT_DIR}/po_parser.cpp
    ${TINYGETTEXT_DIR}/tinygettext.cpp
     
	#Tinyxml2 sources
	${TINYXML_DIR}/tinyxml2.cpp

    #OpenDungeons sources
    ${SRC}/AbstractApplicationMode.cpp
    ${SRC}/AIFactory.cpp
    ${SRC}/AIManager.cpp
    ${SRC}/AIWrapper.cpp
    ${SRC}/ASWrapper.cpp    
    ${SRC}/BaseAI.cpp
    ${SRC}/CameraManager.cpp
    ${SRC}/ChatMessage.cpp
    ${SRC}/Client.cpp
    ${SRC}/ClientNotification.cpp
    ${SRC}/Console.cpp
    ${SRC}/Console_executePromptCommand.cpp
    ${SRC}/Console_getHelp.cpp
    ${SRC}/ConsoleMode.cpp
    ${SRC}/Creature.cpp
    ${SRC}/CreatureAction.cpp
    ${SRC}/CreatureDefinition.cpp
    ${SRC}/CreatureSound.cpp
    ${SRC}/DirectionalTrap.cpp
    ${SRC}/Director.cpp
    ${SRC}/EditorContext.cpp
    ${SRC}/EditorMode.cpp
    ${SRC}/Field.cpp
    ${SRC}/Functions.cpp
    ${SRC}/GameContext.cpp
    ${SRC}/GameEntity.cpp
    ${SRC}/GameMap.cpp
    ${SRC}/GameMode.cpp
    ${SRC}/GameContext.cpp
    ${SRC}/GameStateManager.cpp
    ${SRC}/GoalClaimNTiles.cpp
    ${SRC}/Goal.cpp
    ${SRC}/GoalKillAllEnemies.cpp
    ${SRC}/GoalMineNGold.cpp
    ${SRC}/GoalProtectCreature.cpp
    ${SRC}/GoalProtectDungeonTemple.cpp
    ${SRC}/Gui.cpp
    ${SRC}/LogManager.cpp
    ${SRC}/main.cpp
    ${SRC}/MapLight.cpp
    ${SRC}/MapLoader.cpp
    ${SRC}/MenuMode.cpp
    ${SRC}/MiniMap.cpp
    ${SRC}/MissileObject.cpp
    ${SRC}/ModeContext.cpp
    ${SRC}/ModeManager.cpp
    ${SRC}/MovableGameEntity.cpp
    ${SRC}/MusicPlayer.cpp
    ${SRC}/NullAI.cpp
    ${SRC}/ODApplication.cpp
    ${SRC}/ODFrameListener.cpp
    ${SRC}/Player.cpp
    ${SRC}/ProximityTrap.cpp
    ${SRC}/Quadtree.cpp
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
    ${SRC}/TestAI.cpp
    ${SRC}/TextRenderer.cpp
    ${SRC}/Tile.cpp
#   ${SRC}/TileMap.cpp
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
# TODO - make this set version string in source file automatically.
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 4)
set(${PROJECT_NAME}_PATCH_LEVEL   9)

# Where we want the binary
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

##################################
#### Find packages ###############
##################################

find_package(OIS REQUIRED)
find_package(OGRE REQUIRED)
find_package(CEGUI REQUIRED)
find_package(SFML REQUIRED COMPONENTS Audio System)

#This has to cover the versions not already known by CMake
set(Boost_ADDITIONAL_VERSIONS 1.47 1.47.0 1.47.1)

if(WIN32 AND ${OGRE_FOUND})
	# On windows we can try to use boost from the Ogre SDK instead
    set( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${ENV_OGRE_HOME}/boost_1_44/lib)
    set( CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${ENV_OGRE_HOME}/boost_1_44)

    # We set these instead of searching if using MSVC
    # This is because the libs in the ogre dir has a lib prefix making findboost not find them
	set( OD_BOOST_LIB_DIRS  ${ENV_OGRE_HOME}/boost_1_44/lib)
	set( OD_BOOST_INCLUDE_DIRS ${ENV_OGRE_HOME}/boost_1_44)
endif()

if(MINGW)
    # We need to specify the compiler here that Ogre used to compile boost
    set(Boost_COMPILER -mgw45)
    find_package(Boost COMPONENTS thread REQUIRED)
endif()

#OpenAL
#If we are on windows we can also look for OpenAL where SFML is
if(WIN32 AND ${SFML_FOUND})
    set( CMAKE_LIBRARY_PATH ${SFMLDIR}/extlibs/libs-vc2005 ${SFMLDIR}/extlibs/libs-mingw)
    set( CMAKE_INCLUDE_PATH ${SFMLDIR}/extlibs/headers ${SFMLDIR}/extlibs/headers/AL)
endif()

find_package(OpenAL REQUIRED)

if(UNIX)
    find_package(Threads REQUIRED)
endif()

##################################
#### Headers and linking #########
##################################

#add all project specific include directorieS
include_directories(
    #OpenDungeons includes
    ${CMAKE_SOURCE_DIR}/source

    #General external includes
    ${DEPENDENCIES_DIR}

    #tinygettext includes
    ${TINYGETTEXT_DIR}

	#tinyxml2 includes
	${TINYXML_DIR}

    #Angelscript includes    
    ${ANGELSCRIPT_DIR}/angelscript/include
	${ANGELSCRIPT_SRC_DIR}

    #AngelScript Addons, in-comment only the ones we actually use
#    ${ANGELSCRIPT_ADDON_DIR}/autowrapper
#    ${ANGELSCRIPT_ADDON_DIR}/clib
#    ${ANGELSCRIPT_ADDON_DIR}/contextmgr
#    ${ANGELSCRIPT_ADDON_DIR}/debugger
#    ${ANGELSCRIPT_ADDON_DIR}/scriptany
    ${ANGELSCRIPT_ADDON_DIR}/scriptarray
    ${ANGELSCRIPT_ADDON_DIR}/scriptbuilder
#    ${ANGELSCRIPT_ADDON_DIR}/scriptdictionary
#    ${ANGELSCRIPT_ADDON_DIR}/scriptfile
    ${ANGELSCRIPT_ADDON_DIR}/scripthelper
#    ${ANGELSCRIPT_ADDON_DIR}/scriptmath
#    ${ANGELSCRIPT_ADDON_DIR}/scriptmath3d
    ${ANGELSCRIPT_ADDON_DIR}/scriptstdstring

    #external packages includes
    ${CEGUI_INCLUDE_DIR}
    ${SFML_INCLUDE_DIR}
    ${OGRE_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${OIS_INCLUDE_DIRS}
)

if(WIN32)
    if(MINGW)
        # Boost
        link_libraries(${Boost_LIBRARIES})
        include_directories(${Boost_INCLUDE_DIRS})
    endif()
    
    if(MSVC)
        #Provide dirent.h if visual studio is used
        include_directories(${DEPENDENCIES_DIR}/dirent)
        include_directories(${OD_BOOST_INCLUDE_DIRS})
        link_directories(${OD_BOOST_LIB_DIRS})
    endif()

    #Pthreads on windows
    include_directories(${DEPENDENCIES_DIR}/pthreads/include/)
    link_directories(${DEPENDENCIES_DIR}/pthreads/lib/)
endif()

##################################
#### Binary ######################
##################################

# Create the binary file (WIN32 makes sure there is no console window on windows.)
add_executable(${PROJECT_BINARY_NAME} WIN32 ${OD_SOURCEFILES})

# set path
set(BINDIR games CACHE PATH "")

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
        DESTINATION ${BINDIR}
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

# Link pthreads. (And also userenv if we are on windows.)
if(WIN32)
    if(MSVC_IDE)
        #Set some extra compiler flags
        #TODO - investigate if these are what they should be
        set( PLATFORM_C_FLAGS "/W3 /MD /Od /D \"WIN32\" /D \"_WINDOWS\" /Gm /Gy /fp:fast /ZI /EHsc /SUBSYSTEM:WINDOWS /ENTRY:\"main\"" )
        set( PLATFORM_C_FLAGS_DEBUG "/W3 /MDd /Od /Gm /Gy /fp:fast /ZI /DOD_DEBUG" )

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
    elseif(MINGW)
        target_link_libraries(
            ${PROJECT_BINARY_NAME}

            "libpthreadGC2.a"
            "ws2_32"
            Userenv
        )
        
    endif()
else()
    target_link_libraries (${PROJECT_BINARY_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    #TODO - move compiler defs
    #Add compiler warnings
    #(-fno-strict-aliasing is here to silence a lot of warnings from angelscript)
    add_definitions( "-Wall -Wunused -Wno-deprecated -fno-strict-aliasing" )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-DOD_DEBUG")
endif()

##################################
#### Configure settings files ####
##################################

#Make a copy for use in the build directory, and later one copy for installing.
configure_file(
    ${CMAKE_CONFIG_DIR}/resources.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources.cfg
)

configure_file(
    ${CMAKE_CONFIG_DIR}/resources.zip.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/resources.zip.cfg
)

if(WIN32)
    #On windows, use current directory for plugins and data for now
    set(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL "plugins")
    set(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG "plugins")
    set(OPENDUNGEONS_DATA_PATH ".")
    set(OPENDUNGEONS_DATA_DEST ".")
else()
    #only use d3d renderer plugins if we are on windows
    set(OPENDUNGEONS_COMMENT_D3D "#")
    #On linux, we should look in the standard paths for plugins
    if(${OGRE_PLUGIN_DIR_REL} STREQUAL NOTFOUND)
        #Try pkgconfig if the ogre module doesn't set the plugin paths
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} OGRE --variable=plugindir
                OUTPUT_VARIABLE OD_PLUGIN_DIR_TEMP
                RESULT_VARIABLE OD_PKGCONFIG_FAILED
                ERROR_VARIABLE  OD_PKGCONFIG_ERROR)
            if(OD_PKGCONFIG_FAILED)
                message(STATUS "Failed to run pkgconfig when looking for plugin directory" ${OD_PLUGIN_DIR_TEMP} " Error:" ${OD_PKGCONFIG_ERROR})
                get_filename_component(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ${OGRE_RenderSystem_GL_LIBRARY_REL} PATH)
            else()
                set(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ${OD_PLUGIN_DIR_TEMP})
                set(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG ${OD_PLUGIN_DIR_TEMP})
            endif()
        endif()
    else()
        set(OPENDUNGEONS_OGRE_PLUGIN_DIR_REL ${OGRE_PLUGIN_DIR_REL})
        set(OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG ${OGRE_PLUGIN_DIR_DBG})
    endif()

    message(STATUS "Plugin path rel: " ${OPENDUNGEONS_OGRE_PLUGIN_DIR_REL})
    message(STATUS "Plugin path dbg: " ${OPENDUNGEONS_OGRE_PLUGIN_DIR_DBG})

    #Set data location to where we install if we want to use the installer
    set(OPENDUNGEONS_DATA_DEST share/games/${PROJECT_NAME})
    set(OPENDUNGEONS_DATA_PATH ${CMAKE_INSTALL_PREFIX}/${OPENDUNGEONS_DATA_DEST} CACHE PATH "")
endif()

#Do the configuration
configure_file(${CMAKE_CONFIG_DIR}/plugins.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins.cfg)
configure_file(${CMAKE_CONFIG_DIR}/plugins_d.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/plugins_d.cfg)
configure_file(${CMAKE_CONFIG_DIR}/resources.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg)
configure_file(${CMAKE_CONFIG_DIR}/resources.zip.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/resources_install.zip.cfg)
if(UNIX)
    #On linux, set up the launcher script
    #TODO - add key repeat thing
	set(OPENDUNGEONS_BINARY_PATH ${CMAKE_INSTALL_PREFIX}/games/${PROJECT_BINARY_NAME} CACHE PATH "")
	configure_file(${CMAKE_CONFIG_DIR}/OpenDungeons.in ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons)
endif()

##################################
#### Installation ################
##################################

#(This is currently not used)

set(OPENDUNGEONS_CONFIGFILES ${CMAKE_CURRENT_SOURCE_DIR}/plugins.cfg)

install(FILES ${OPENDUNGEONS_CONFIGFILES} DESTINATION ${OPENDUNGEONS_DATA_DEST})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.cfg)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources_install.zip.cfg DESTINATION ${OPENDUNGEONS_DATA_DEST} RENAME resources.zip.cfg)

if(NOT WIN32)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenDungeons DESTINATION ${BINDIR} PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

##################################
#### Copy files to binary dir ####
##################################

# Copy level and scripts dir from git to build dir if this is not an in-source build
if( NOT ${CMAKE_CURRENT_SOURCE_DIR} EQUAL ${CMAKE_CURRENT_BINARY_DIR})
    configure_file(levels_git/Test.level ${CMAKE_CURRENT_BINARY_DIR}/levels_git/Test.level)
    file(COPY scripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

##################################
#### Packaging ###################
##################################

#Not used at the moment, more flexible to do the deb packages manually
#TODO - set up for making tarballs
set(CPACK_SOURCE_IGNORE_FILES "~$" "${CMAKE_SOURCE_DIR}.*/.svn/" "${CMAKE_SOURCE_DIR}/debian/")
set(CPACK_PACKAGE_NAME opendungeons)
set(CPACK_PACKAGE_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_LEVEL})
set(CPACK_PACKAGE_CONTACT OpenDungeons Team)
set(CPACK_GENERATOR "DEB")
set(CPACK_SOURCE_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcegui (>= 0.7.1), libois-1.2.0, libsfml-audio1.6, libopenal1, libogremain-1.7.2")
include(CPack)
