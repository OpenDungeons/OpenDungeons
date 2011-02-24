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
