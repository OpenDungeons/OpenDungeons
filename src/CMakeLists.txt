SET(
     SRC
     ActiveObject.cpp
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
     MusicPlayer.cpp
     Player.cpp
     RenderRequest.cpp
     Room.cpp
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
     TextRenderer.cpp
     Tile.cpp
     Trap.cpp
     TrapCannon.cpp
     Weapon.cpp
   )

# Create the binary file
ADD_EXECUTABLE( ${PROJECT_BINARY_NAME} WIN32 ${SRC} )

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
