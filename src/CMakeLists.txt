##################################
#### Source files ################
##################################

set(
     OD_SOURCEFILES
     ActiveObject.cpp
     AttackableObject.cpp
     AnimatedObject.cpp
     ButtonHandlers.cpp
     ChatMessage.cpp
     Client.cpp
     ClientNotification.cpp
     CreatureAction.cpp
     CreatureClass.cpp
     Creature.cpp
     CreatureSound.cpp
     ExampleFrameListener.cpp
     Field.cpp
     Functions.cpp
     GameMap.cpp
     GoalClaimNTiles.cpp
     Goal.cpp
     GoalKillAllEnemies.cpp
     GoalMineNGold.cpp
     GoalProtectCreature.cpp
     GoalProtectDungeonTemple.cpp
     main.cpp
     MapEditor.cpp
     MapLight.cpp
     MissileObject.cpp
     MusicPlayer.cpp
     Player.cpp
     RadialVector2.cpp
     RenderRequest.cpp
     Room.cpp
     RoomDojo.cpp
     RoomDungeonTemple.cpp


     RoomForge.cpp
     RoomObject.cpp
     RoomPortal.cpp
     RoomQuarters.cpp
     RoomTreasury.cpp
     Seat.cpp
     Server.cpp
     ServerNotification.cpp
     Socket.cpp
     SoundEffectsHelper.cpp
     Spell.cpp
     TextRenderer.cpp
     Tile.cpp
     TileCoordinateMap.cpp
     Trap.cpp
     TrapCannon.cpp
     Weapon.cpp
   )



##################################
#### Library/include directories #
##################################

# CEGUI
# FIXME This should be handled by the find script

#Add cegui library dirs.
link_directories(${CEGUI_LIB_DIR})

link_directories(${CEGUIOGRE_LIB_DIR})
include_directories(${CEGUI_INCLUDE_DIR})

# Boost
link_libraries(${Boost_LIBRARIES})
include_directories(${Boost_INCLUDE_DIRS})

#includes only
include_directories(${SFML_INCLUDE_DIR} ${OGRE_INCLUDE_DIRS} 
    ${OPENAL_INCLUDE_DIR} ${OIS_INCLUDE_DIRS})

#TODO - should we have some error handling here?

# FIXME workaround , for some reason the include 
# path got chopped off at the end when generating eclipse projects
# not sure if i should change this back or not
# TODO - check if this is still needed
if(MINGW)
	set(SFMLINCLUDEFIX ${SFML_AUDIO_INCLUDE_DIR}clude)
	include_directories(${SFMLINCLUDEFIX})
endif()

#Pthreads on windows
if(WIN32)
    set( PTHREADS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/dep/pthreads/include/" )
    include_directories( ${PTHREADS_INCLUDE_DIR} )
    set( PTHREADS_LIBRARY_DIRS "${CMAKE_SOURCE_DIR}/dep/pthreads/lib/" )
    include_directories( ${PTHREADS_LIBRARY_DIRS} )
endif()

##################################
#### Binary ######################
##################################
   
# Create the binary file (WIN32 makes sure there is no console window on windows.)
add_executable( ${PROJECT_BINARY_NAME} WIN32 ${OD_SOURCEFILES} )

##################################
#### Install targets #############
##################################

# Add install target
if( WIN32 )
	install(TARGETS ${PROJECT_BINARY_NAME}
		DESTINATION .
	) 
else () #Linux/BSD
	install(TARGETS ${PROJECT_BINARY_NAME}
		DESTINATION games
	)
endif() 

##################################
#### Link libraries ##############
##################################

# Link libraries
target_link_libraries(${PROJECT_BINARY_NAME} ${OGRE_LIBRARIES} 
    ${CEGUI_LIBRARIES} ${CEGUIOGRE_LIBRARIES} ${OPENAL_LIBRARY}
    ${OIS_LIBRARIES} ) 
    
# MSVC automatically links boost
if(NOT MSVC)
    target_link_libraries(${PROJECT_BINARY_NAME} ${Boost_LIBRARIES})
endif()

# Link sfml
# Use debug libraries if they exist
if(${SFML_AUDIO_LIBRARY_SHARED_DEBUG})
target_link_libraries(${PROJECT_BINARY_NAME} 
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

#TODO - move compiler defs
if( NOT WIN32 )
  # Add compiler warnings
    add_definitions( "-Wall -Wunused -ansi -Wno-deprecated" )
endif( )
# Link pthreads and shlwapi if we are on windows
if( WIN32 )


    if( MSVC_IDE )
    #TODO - can we remove this?
#        IF( EXISTS $ENV{OGRE_HOME} )
#            SET( OGRESDK $ENV{OGRE_HOME} )
#            STRING( REGEX REPLACE "[\\]" "/" OGRESDK "${OGRESDK}" )
#            MESSAGE( STATUS "Adding Additional Link Directories" )
#            LINK_DIRECTORIES( ${OGRESDK}/lib )
#        ENDIF( )

        #Set some extra compiler flags
        #TODO - investigate if these are what they should be
        set( PLATFORM_C_FLAGS "/W3 /MD /Od /D \"WIN32\" /D \"_WINDOWS\" /Gm /Gy /fp:fast /ZI /EHsc /SUBSYSTEM:WINDOWS /ENTRY:\"main\"" )
        set( PLATFORM_C_FLAGS_DEBUG "/W3 /MDd /Od /Gm /Gy /fp:fast /ZI" )

        set( CMAKE_CXX_FLAGS "${PLATFORM_C_FLAGS}" )
        set( CMAKE_CXX_FLAGS_RELEASE "${PLATFORM_C_FLAGS}" )
        set( CMAKE_CXX_FLAGS_DEBUG "${PLATFORM_C_FLAGS_DEBUG}" )

        #Link to pthreads
        target_link_libraries(${PROJECT_BINARY_NAME} "pthreadVC2.lib" )
        #link to shlwapi to get appdata path
        target_link_libraries(${PROJECT_BINARY_NAME} "ws2_32.lib" shlwapi)
        message( STATUS "Using win pthreads" )
    endif()
    if(MINGW)
        target_link_libraries(${PROJECT_BINARY_NAME} "libpthreadGC2.a" )
        target_link_libraries(${PROJECT_BINARY_NAME} "ws2_32" shlwapi )
    endif( )
endif()


