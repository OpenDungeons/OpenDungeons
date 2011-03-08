SET(
     SRC
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

#FIXME this should be in the find script
find_package(CEGUI REQUIRED)
link_directories(${CEGUI_LIB_DIR})
find_package(CEGUIOGRE REQUIRED)
link_directories(${CEGUIOGRE_LIB_DIR})
include_directories(${CEGUI_INCLUDE_DIR})


#find_package(CEGUIOGRE REQUIRED)
#if( MSVC )
#SET( LIBSUFFIX "lib" )
#else ()
#SET( LIBSUFFIX "a" )
#endif()
#target_link_libraries(${PROJECT_BINARY_NAME} debug "${CEGUI_LIB_DIR}/CEGUIBase_d.${LIBSUFFIX}" optimized 
#	"${CEGUI_LIB_DIR}/CEGUIBase.${LIBSUFFIX}")
#target_link_libraries(${PROJECT_BINARY_NAME} debug "${CEGUI_LIB_DIR}/CEGUIOgreRenderer_d.${LIBSUFFIX}" optimized 
#	"${CEGUI_LIB_DIR}/CEGUIOgreRenderer.${LIBSUFFIX}")
#FIXME - we are doing this twice
find_package(OGRE REQUIRED)
   #Find boost and link 
#Needs to be updated with use of new boost release
set(Boost_ADDITIONAL_VERSIONS 1.42.0 1.42 1.43.0 1.43 1.44.0 1.44 1.45.0 1.45) 
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS   OFF)
if( WIN32 )
	include_directories(${ENV_OGRE_HOME}/boost_1_44)
#		set(BOOST_ROOT ${ENV_OGRE_HOME}/boost_1_44)
	LINK_DIRECTORIES(${ENV_OGRE_HOME}/boost_1_44/lib)
	MESSAGE( STATUS ${ENV_OGRE_HOME} )
endif()
find_package( Boost COMPONENTS thread)
#if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
#	if ( WIN32 )
#		link_directories(${Boost_LIBRARY_DIRS}) ##Needed because VS wants to autolink
#	endif()
if(Boost_FOUND)
	link_libraries(${Boost_LIBRARIES})
endif()
#TODO - should we have some error handling here?
#Todo- move this to where the other link dirs are specified
   
   
# Create the binary file
ADD_EXECUTABLE( ${PROJECT_BINARY_NAME} WIN32 ${SRC} )
# Add install target

if( WIN32 )
	install(TARGETS ${PROJECT_BINARY_NAME}
		DESTINATION .
	) 
else () #Unix
	install(TARGETS ${PROJECT_BINARY_NAME}
		DESTINATION games
	)
endif() 


# This foreach loads all of the packages specified as required.
FOREACH( Package ${Required_Packages} )
  FIND_PACKAGE( ${Package} REQUIRED )
  IF( ${Package}_FOUND )
    INCLUDE_DIRECTORIES( ${${Package}_INCLUDE_DIR} )
    IF( ${Package}_LIBRARIES )
      TARGET_LINK_LIBRARIES( ${PROJECT_BINARY_NAME} ${${Package}_LIBRARIES} )
    ELSE( )
      LINK_DIRECTORIES( ${${Package}_LIBRARY_DIRS} )
	#LINK_DIRECTORIES( ${${Package}_LIBRARY_DIR} )
    ENDIF( )
  ELSE( )
    MESSAGE( STATUS "${Package} not found" )
  ENDIF( )
ENDFOREACH( )

#Link cegui
target_link_libraries(${PROJECT_BINARY_NAME} ${CEGUI_LIBRARIES})
target_link_libraries(${PROJECT_BINARY_NAME} ${CEGUIOGRE_LIBRARIES})

#Link and include sfml
find_package(SFML REQUIRED COMPONENTS Audio System)

target_link_libraries(${PROJECT_BINARY_NAME} optimized ${SFML_AUDIO_LIBRARY_SHARED_NONDEBUG}
	optimized ${SFML_SYSTEM_LIBRARY_SHARED_NONDEBUG}
	debug ${SFML_AUDIO_LIBRARY_SHARED_DEBUG} debug ${SFML_SYSTEM_LIBRARY_SHARED_DEBUG}) 






	
include_directories(${SFML_INCLUDE_DIR})
#FIXME workaround , for some reason the include 
#path got chopped off at the end when generating eclipse projects
#not sure if i should change this back or not
#TODO - check if this is still needed
if(MINGW)
	set(SFMLINCLUDEFIX ${SFML_AUDIO_INCLUDE_DIR}clude)
	include_directories(${SFMLINCLUDEFIX})
endif()

if( UNIX )
#Needed for gentoo as the libraries are build with "--as-needed", possibly also other distros
#	link_libraries(${SFML_SYSTEM_LIBRARY_SHARED_NONDEBUG})
endif()
