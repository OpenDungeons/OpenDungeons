/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "EditorMode.h"

#include "MapLoader.h"
#include "GameMap.h"
#include "Socket.h"
#include "Network.h"
#include "ClientNotification.h"
#include "ODFrameListener.h"
#include "LogManager.h"
#include "Gui.h"
#include "ODApplication.h"
#include "ResourceManager.h"
#include "TextRenderer.h"
#include "Creature.h"
#include "MapLight.h"
#include "Seat.h"
#include "Trap.h"
#include "Player.h"
#include "RenderRequest.h"
#include "RenderManager.h"
#include "CameraManager.h"
#include "Console.h"

#include <OgreEntity.h>

#include <algorithm>
#include <vector>
#include <string>

EditorMode::EditorMode(ModeContext *modeContext):
    AbstractApplicationMode(modeContext, ModeManager::EDITOR),
    mChanged(false),
    mCurrentFullness(100),
    mCurrentTileRadius(1),
    mBrushMode(false),
    mCurrentTileType(Tile::dirt),
    mDragType(nullDragType)
{
}

EditorMode::~EditorMode()
{
}

bool EditorMode::mouseMoved(const OIS::MouseEvent &arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
    //TODO: do this (and the others isInGame() in here) by GameState

    if (!mMc->frameListener->isTerminalActive())
    {
        //If we have a room or trap (or later spell) selected, show what we
        //have selected
        //TODO: This should be changed, or combined with an icon or something later.
        if (mMc->gameMap->getLocalPlayer()->getNewRoomType() || mMc->gameMap->getLocalPlayer()->getNewTrapType())
        {
            TextRenderer::getSingleton().moveText(ODApplication::POINTER_INFO_STRING,
                                                  (Ogre::Real)(arg.state.X.abs + 30), (Ogre::Real)arg.state.Y.abs);
        }

        Ogre::RaySceneQueryResult& result = ODFrameListener::getSingleton().doRaySceneQuery(arg);

        Ogre::RaySceneQueryResult::iterator itr = result.begin();
        Ogre::RaySceneQueryResult::iterator end = result.end();
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();

        std::string resultName;

        if(mMc->mDragType == rotateAxisX)
        {
            mMc->frameListener->cm->move(CameraManager::randomRotateX, arg.state.X.rel);
        }
        else if(mMc->mDragType == rotateAxisY)
        {
            mMc->frameListener->cm->move(CameraManager::randomRotateY, arg.state.Y.rel);
        }
        else if (mMc->mDragType == tileSelection || mMc->mDragType == addNewRoom || mMc->mDragType == nullDragType)
        {
            // Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
            for (; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();

                    if (resultName.find("Level_") != std::string::npos)
                    {
                        //Make the mouse light follow the mouse
                        //TODO - we should make a pointer to the light or something.
                        Ogre::RaySceneQuery* rq = mMc->frameListener->getRaySceneQuery();
                        Ogre::Real dist = itr->distance;
                        Ogre::Vector3 point = rq->getRay().getPoint(dist);
                        mSceneMgr->getLight("MouseLight")->setPosition(point.x, point.y, 4.0);

                        // Get the x-y coordinates of the tile.

                        //warning obsolete cstdio function
                        sscanf(resultName.c_str(), "Level_%i_%i", &mMc->xPos, &mMc->yPos);
                        RenderRequest *request = new RenderRequest;

                        request->type = RenderRequest::showSquareSelector;
                        request->p = static_cast<void*>(&mMc->xPos);
                        request->p2 = static_cast<void*>(&mMc->yPos);

                        // Add the request to the queue of rendering operations to be performed before the next frame.
                        RenderManager::queueRenderRequest(request);


                        // Make sure the "square selector" mesh is visible and position it over the current tile.

                        //mSceneMgr->getLight("MouseLight")->setPosition(mMc->xPos, mMc->yPos, 2.0);

                        if (mMc->mLMouseDown)
                        {
                            // Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
                            //TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords rather than the reverse that it is doing now.
                            std::vector<Tile*> affectedTiles = mMc->gameMap->rectangularRegion(mMc->xPos,
                                                        mMc->yPos, mMc->mLStartDragX, mMc->mLStartDragY);

                            for (int jj = 0; jj < mMc->gameMap->getMapSizeY(); ++jj)
                            {
                                for (int ii = 0; ii < mMc->gameMap->getMapSizeX(); ++ii)
                                {
                                    mMc->gameMap->getTile(ii,jj)->setSelected(false,mMc->gameMap->getLocalPlayer());
                                }
                            }


                            for( std::vector<Tile*>::iterator itr =  affectedTiles.begin(); itr != affectedTiles.end(); ++itr )
                            {

                            //   for (int jj = 0; jj < GameMap::getMapSizeY(); ++jj)
                            //   {
                            //       for (int ii = 0; ii < GameMap::getMapSizeX(); ++ii)
                            //       {
                            //            for (TileMap_t::iterator itr = mMc->gameMap->firstTile(), last = mMc->gameMap->lastTile();
                            //                itr != last; ++itr)
                            //            {
                            //            }
                            //
                            //          With the above, I see no reason aditional iteration over tiles in GameMap
                            //          if (std::find(affectedTiles.begin(), affectedTiles.end(), mMc->gameMap->getTile(ii,jj)) != affectedTiles.end())
                            //          {
                            //              (*itr)->setSelected(true, mMc->gameMap->getLocalPlayer());
                            //          }
                            //          else
                            //          {
                            //              (*itr)->setSelected(false, mMc->gameMap->getLocalPlayer());
                            //          }
                            //          }
                            //     }
                                (*itr)->setSelected(true, mMc->gameMap->getLocalPlayer());
                            }
                        }

                        if (mMc->mRMouseDown)
                        {
                        }

                        break;
                    }
                }
            }
        }
        else
        {
            // We are dragging a creature but we want to loop over the result set to find the first tile entry,
            // we do this to get the current x-y location of where the "square selector" should be drawn.
            for (; itr != end; ++itr)
            {
                if (itr->movable != NULL)
                {
                    // Check to see if the current query result is a tile.
                    resultName = itr->movable->getName();

                    if (resultName.find("Level_") != std::string::npos)
                    {
                        // Get the x-y coordinates of the tile.
                        sscanf(resultName.c_str(), "Level_%i_%i", &mMc->xPos, &mMc->yPos);

                        RenderRequest *request = new RenderRequest;

                        request->type = RenderRequest::showSquareSelector;
                        request->p = static_cast<void*>(&mMc->xPos);
                        request->p2 = static_cast<void*>(&mMc->yPos);
                        // Make sure the "square selector" mesh is visible and position it over the current tile.
                    }
                }
            }
        }

        // If we are drawing with the brush in the map editor.
        if (mMc->mLMouseDown && mMc->mDragType == tileBrushSelection && !isInGame())
        {
            // Loop over the square region surrounding current mouse location and either set the tile type of the affected tiles or create new ones.
            Tile *currentTile;
            std::vector<Tile*> affectedTiles;
            int radiusSquared = mCurrentTileRadius * mCurrentTileRadius;

            for (int i = -1 * (mCurrentTileRadius - 1); i <= (mCurrentTileRadius
                    - 1); ++i)
            {
                for (int j = -1 * (mCurrentTileRadius - 1); j
                        <= (mCurrentTileRadius - 1); ++j)
                {
                    // Check to see if the current location falls inside a circle with a radius of mCurrentTileRadius.
                    int distSquared = i * i + j * j;

                    if (distSquared > radiusSquared)
                        continue;

                    currentTile = mMc->gameMap->getTile(mMc->xPos + i, mMc->yPos + j);

                    // Check to see if the current tile already exists.
                    if (currentTile != NULL)
                    {
                        // It does exist so set its type and fullness.
                        affectedTiles.push_back(currentTile);
                        currentTile->setType((Tile::TileType)mCurrentTileType);
                        currentTile->setFullness((Tile::TileType)mCurrentFullness);
                    }
                    else
                    {
                        // The current tile does not exist so we need to create it.
                        //currentTile = new Tile;
                        stringstream ss;

                        ss.str(std::string());
                        ss << "Level";
                        ss << "_";
                        ss << mMc->xPos + 1;
                        ss << "_";
                        ss << mMc->yPos + 1;

                        currentTile = new Tile(mMc->xPos + i, mMc->yPos + j,
                                               (Tile::TileType)mCurrentTileType, (Tile::TileType)mCurrentFullness);
                        currentTile->setName(ss.str());
                        mMc->gameMap->addTile(currentTile);
                        currentTile->createMesh();
                    }
                }
            }

            // Add any tiles which border the affected region to the affected tiles list
            // as they may alo want to switch meshes to optimize polycount now too.
            std::vector<Tile*> borderingTiles = mMc->gameMap->tilesBorderedByRegion(
                                                    affectedTiles);

            affectedTiles.insert(affectedTiles.end(), borderingTiles.begin(),
                                 borderingTiles.end());

            // Loop over all the affected tiles and force them to examine their
            // neighbors.  This allows them to switch to a mesh with fewer
            // polygons if some are hidden by the neighbors.
            for (unsigned int i = 0; i < affectedTiles.size(); ++i)
                affectedTiles[i]->setFullness(affectedTiles[i]->getFullness());
        }

        // If we are dragging a map light we need to update its position to the current x-y location.
        if (mMc->mLMouseDown && mMc->mDragType == mapLight && !isInGame())
        {
            MapLight* tempMapLight = mMc->gameMap->getMapLight(mDraggedMapLight);

            if (tempMapLight != NULL)
                tempMapLight->setPosition((Ogre::Real)mMc->xPos, (Ogre::Real)mMc->yPos, tempMapLight->getPosition().z);
        }

        if (arg.state.Z.rel > 0)
        {
            if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
            {
                mMc->gameMap->getLocalPlayer()->rotateCreaturesInHand(1);
            }
            else
            {
                mMc->frameListener->cm->move(CameraManager::moveDown);
            }
        }
        else
            if (arg.state.Z.rel < 0)
            {
                if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Ctrl))
                {
                    mMc->gameMap->getLocalPlayer()->rotateCreaturesInHand(-1);
                }
                else
                {
                    mMc->frameListener->cm->move(CameraManager::moveUp);
                }
            }
            else
            {
                mMc->frameListener->cm->stopZooming();
            }
    }

    //mMc->frameListener->cm->move(CameraManager::fullStop);

    if (!(mMc->directionKeyPressed || mMc->mDragType == rotateAxisX || mMc->mDragType == rotateAxisY))
    {

        if (arg.state.X.abs == 0)
            mMc->frameListener->cm->move(CameraManager::moveLeft);
        else
            mMc->frameListener->cm->move(CameraManager::stopLeft);

        if (arg.state.X.abs ==  arg.state.width)
            mMc->frameListener->cm->move(CameraManager::moveRight);
        else
            mMc->frameListener->cm->move(CameraManager::stopRight);

        if (arg.state.Y.abs == 0)
            mMc->frameListener->cm->move(CameraManager::moveForward);
        else
            mMc->frameListener->cm->move(CameraManager::stopForward);

        if (arg.state.Y.abs ==  arg.state.height)
            mMc->frameListener->cm->move(CameraManager::moveBackward);
        else
            mMc->frameListener->cm->move(CameraManager::stopBackward);
    }

    //  cerr << arg.state.X.abs <<" " ;
    //  cerr << arg.state.Y.abs <<" " ;
    //  cerr << arg.state.Z.abs <<"\n" ;
    //  cerr << arg.state.width <<"\n" ;
    //  cerr << arg.state.height <<"\n" ;

    return true;
}

bool EditorMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::GUIContext& ctxt = CEGUI::System::getSingleton().getDefaultGUIContext();
    ctxt.injectMouseButtonDown(Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press is on a CEGUI window ignore it
    CEGUI::Window *tempWindow = ctxt.getWindowContainingMouse();

    if (tempWindow != 0 && tempWindow->getName().compare("EDITORGUI") != 0)
    {
        mMc->mouseDownOnCEGUIWindow = true;
        return true;
    }

    mMc->mouseDownOnCEGUIWindow = false;

    Ogre::RaySceneQueryResult &result = ODFrameListener::getSingleton().doRaySceneQuery(arg);
    Ogre::RaySceneQueryResult::iterator itr = result.begin();

    std::string resultName;

    // Left mouse button down

    if (id == OIS::MB_Left)
    {
        mMc->mLMouseDown = true;
        mMc->mLStartDragX = mMc->xPos;
        mMc->mLStartDragY = mMc->yPos;

        if(arg.state.Y.abs < 0.1*arg.state.height || arg.state.Y.abs > 0.9*arg.state.height)
        {
            mMc->mDragType=rotateAxisX;
            return true;
        }
        else if(arg.state.X.abs > 0.9*arg.state.width || arg.state.X.abs < 0.1*arg.state.width)
        {
            mMc->mDragType=rotateAxisY;
            return true;
        }

        // See if the mouse is over any creatures
        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("Creature_") != std::string::npos)
                {
                    // Begin dragging the creature
                    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
                    mSceneMgr->getEntity("SquareSelector")->setVisible(
                        false);

                    mDraggedCreature = resultName.substr(
                                            ((std::string) "Creature_").size(),
                                            resultName.size());
                    Ogre::SceneNode *node = mSceneMgr->getSceneNode(
                                                mDraggedCreature + "_node");
                    ODFrameListener::getSingleton().getCreatureSceneNode()->removeChild(node);
                    mSceneMgr->getSceneNode("Hand_node")->addChild(node);
                    node->setPosition(0, 0, 0);
                    mMc->mDragType = creature;

                    SoundEffectsHelper::getSingleton().playInterfaceSound(
                        SoundEffectsHelper::PICKUP);

                    return true;
                }

            }

            ++itr;
        }

        // If no creatures are under the  mouse run through the list again to check for lights
        //FIXME: These other code blocks that loop over the result list should probably use this same loop structure.
        itr = result.begin();

        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("MapLightIndicator_") != std::string::npos)
                {
                    mMc->mDragType = mapLight;
                    mDraggedMapLight = resultName.substr(
                                            ((std::string) "MapLightIndicator_").size(),
                                            resultName.size());

                    SoundEffectsHelper::getSingleton().playInterfaceSound(
                        SoundEffectsHelper::PICKUP);

                    return true;
                }
            }

            ++itr;
        }

        // If no creatures or lights are under the  mouse run through the list again to check for tiles
        itr = result.begin();

        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                if (resultName.find("Level_") != std::string::npos)
                {
                    // Start by assuming this is a tileSelection drag.
                    mMc->mDragType = tileSelection;

                    // If we are in the map editor, use a brush selection if it has been activated.

                    if (!isInGame() && mBrushMode)
                    {
                        mMc->mDragType = tileBrushSelection;
                    }

                    // If we have selected a room type to add to the map, use a addNewRoom drag type.
                    if (mMc->gameMap->getLocalPlayer()->getNewRoomType() != Room::nullRoomType)
                    {
                        mMc->mDragType = addNewRoom;
                    }

                    // If we have selected a trap type to add to the map, use a addNewTrap drag type.
                    else
                        if (mMc->gameMap->getLocalPlayer()->getNewTrapType() != Trap::nullTrapType)
                        {
                            mMc->mDragType = addNewTrap;
                        }

                    break;
                }
            }

            ++itr;
        }


    }

    // Right mouse button down
    if (id == OIS::MB_Right)
    {
        mMc->mRMouseDown = true;
        mMc->mRStartDragX = mMc->xPos;
        mMc->mRStartDragY = mMc->yPos;

        // Stop creating rooms, traps, etc.
        mMc->mDragType = nullDragType;
        mMc->gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
        mMc->gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
        TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "");

        // If we right clicked with the mouse over a valid map tile, try to drop a creature onto the map.
        Tile *curTile = mMc->gameMap->getTile(mMc->xPos, mMc->yPos);

        if (curTile != NULL)
        {
            mMc->gameMap->getLocalPlayer()->dropCreature(curTile);

            if (mMc->gameMap->getLocalPlayer()->numCreaturesInHand() > 0)
            {
                SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::DROP);
            }
        }
    }

    if (id == OIS::MB_Middle)
    {
        // See if the mouse is over any creatures
        while (itr != result.end())
        {
            if (itr->movable != NULL)
            {
                resultName = itr->movable->getName();

                if (resultName.find("Creature_") != std::string::npos)
                {
                    Creature* tempCreature = mMc->gameMap->getCreature(resultName.substr(
                                                 ((std::string) "Creature_").size(), resultName.size()));

                    if (tempCreature != NULL)
                    {
                        tempCreature->createStatsWindow();
                    }

                    return true;
                }
            }

            ++itr;
        }
    }
    return true;
}


bool EditorMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(Gui::getSingletonPtr()->convertButton(id));

    // If the mouse press was on a CEGUI window ignore it
    if (mMc->mouseDownOnCEGUIWindow)
        return true;

    for (int jj = 0; jj < mMc->gameMap->getMapSizeY(); ++jj)
    {
        for (int ii = 0; ii < mMc->gameMap->getMapSizeX(); ++ii)
        {
            mMc->gameMap->getTile(ii,jj)->setSelected(false, mMc->gameMap->getLocalPlayer());
        }
    }

    // Unselect all tiles
    // for (TileMap_t::iterator itr = mMc->gameMap->firstTile(), last = mMc->gameMap->lastTile();
    //         itr != last; ++itr)
    // {
    //     itr->second->setSelected(false,mMc->gameMap->getLocalPlayer());
    // }

    // Right mouse button up
    if (id == OIS::MB_Right)
    {
        mMc->mRMouseDown = false;
        return true;
    }

    // Left mouse button up
    if (!id == OIS::MB_Left)
        return true;

    if(mMc->mDragType == rotateAxisX)
    {
        mMc->mDragType=nullDragType;
    }
    else if(mMc->mDragType == rotateAxisY)
    {
        mMc->mDragType=nullDragType;
    }

    // Check to see if we are moving a creature
    else if (mMc->mDragType == creature)
    {
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        Ogre::SceneNode *node = mSceneMgr->getSceneNode(mDraggedCreature + "_node");
        mSceneMgr->getSceneNode("Hand_node")->removeChild(node);
        ODFrameListener::getSingleton().getCreatureSceneNode()->addChild(node);
        mMc->mDragType = nullDragType;
        mMc->gameMap->getCreature(mDraggedCreature)->setPosition(Ogre::Vector3((Ogre::Real)mMc->xPos, (Ogre::Real)mMc->yPos, (Ogre::Real)0));
    }

    // Check to see if we are dragging a map light.
    else
    {
        if (mMc->mDragType == mapLight)
        {
            MapLight *tempMapLight = mMc->gameMap->getMapLight(mDraggedMapLight);

            if (tempMapLight != NULL)
            {
                tempMapLight->setPosition((Ogre::Real)mMc->xPos, (Ogre::Real)mMc->yPos, tempMapLight->getPosition().z);
            }
        }

        // Check to see if we are dragging out a selection of tiles or creating a new room
        else
            if (mMc->mDragType == tileSelection || mMc->mDragType == addNewRoom ||
                    mMc->mDragType == addNewTrap)
            {
                //TODO: move to own function.

                // Loop over the valid tiles in the affected region.  If we are doing a tileSelection (changing the tile type and fullness) this
                // loop does that directly.  If, instead, we are doing an addNewRoom, this loop prunes out any tiles from the affectedTiles vector
                // which cannot have rooms placed on them, then if the player has enough gold, etc to cover the selected tiles with the given room
                // the next loop will actually create the room.  A similar pruning is done for traps.
                std::vector<Tile*> affectedTiles = mMc->gameMap->rectangularRegion(mMc->xPos,
                                                    mMc->yPos, mMc->mLStartDragX, mMc->mLStartDragY);
                std::vector<Tile*>::iterator itr = affectedTiles.begin();

                while (itr != affectedTiles.end())
                {
                    Tile *currentTile = *itr;

                    // If we are dragging out tiles.

                    if (mMc->mDragType == tileSelection)
                    {
                        // In the map editor:  Fill the current tile with the new value
                        currentTile->setType((Tile::TileType)mCurrentTileType);
                        currentTile->setFullness((Tile::TileType)mCurrentFullness);
                    }
                    else // if(mMc->mDragType == ExampleFrameListener::addNewRoom || mMc->mDragType == ExampleFrameListener::addNewTrap)
                    {
                        // If the tile already contains a room, prune it from the list of affected tiles.
                        if (!currentTile->isBuildableUpon())
                        {
                            itr = affectedTiles.erase(itr);
                            continue;
                        }

                        // If the currentTile is not empty and claimed, then remove it from the affectedTiles vector.
                        if (!(currentTile->getFullness() < 1
                                && currentTile->getType() == Tile::claimed))
                        {
                            itr = affectedTiles.erase(itr);
                            continue;
                        }
                    }

                    ++itr;
                }

                // If we are adding new rooms the above loop will have pruned out the tiles not eligible
                // for adding rooms to.  This block then actually adds rooms to the remaining tiles.
                if (mMc->mDragType == addNewRoom && !affectedTiles.empty())
                {
                    Room* newRoom = Room::buildRoom(mMc->gameMap, mMc->gameMap->getLocalPlayer()->getNewRoomType(),
                                                    affectedTiles, mMc->gameMap->getLocalPlayer(), true);

                    if (newRoom == NULL)
                    {
                        //TODO:  play sound or something.
                    }
                }

                // If we are adding new traps the above loop will have pruned out the tiles not eligible
                // for adding traps to.  This block then actually adds traps to the remaining tiles.
                else
                    if (mMc->mDragType == addNewTrap && !affectedTiles.empty())
                    {
                        Trap* newTrap = Trap::buildTrap(mMc->gameMap, mMc->gameMap->getLocalPlayer()->getNewTrapType(),
                                                        affectedTiles, mMc->gameMap->getLocalPlayer(), true);

                        if (newTrap == NULL)
                        {
                            //TODO:  play sound or something.
                        }
                    }

                // Add the tiles which border the affected region to the affectedTiles vector since they may need to have their meshes changed.
                std::vector<Tile*> borderTiles = mMc->gameMap->tilesBorderedByRegion(
                                                        affectedTiles);

                affectedTiles.insert(affectedTiles.end(), borderTiles.begin(),
                                        borderTiles.end());

                // Loop over all the affected tiles and force them to examine their neighbors.  This allows
                // them to switch to a mesh with fewer polygons if some are hidden by the neighbors, etc.
                for (itr = affectedTiles.begin(); itr != affectedTiles.end() ; ++itr)
                {
        (*itr)->refreshMesh();

        }
            }
    }

    mMc->mLMouseDown = false;

    return true;
}

bool EditorMode::keyPressed(const OIS::KeyEvent &arg)
{
   //TODO: do this (and the others isInGame() in here) by GameState
    if (mMc->frameListener->isTerminalActive())
        return true;

        // Inject key to Gui
        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan) arg.key);
        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(arg.text);

        CameraManager& camMgr = *(mMc->frameListener->cm);
        switch (arg.key)
        {
        case OIS::KC_F11:
            ODFrameListener::getSingleton().toggleDebugInfo();
            break;

        case OIS::KC_GRAVE:
        case OIS::KC_F12:
            progressMode(ModeManager::CONSOLE);
            mMc->frameListener->setTerminalActive(true);
            Console::getSingleton().setVisible(true);
            mMc->mKeyboard->setTextTranslation(OIS::Keyboard::Ascii);
            break;

        case OIS::KC_LEFT:
        case OIS::KC_A:
            mMc->directionKeyPressed = true;
            camMgr.move(camMgr.moveLeft); // Move left
            break;

        case OIS::KC_RIGHT:
        case OIS::KC_D:
            mMc->directionKeyPressed = true;
            camMgr.move(camMgr.moveRight); // Move right
            break;

        case OIS::KC_UP:
        case OIS::KC_W:
            mMc->directionKeyPressed = true;
            camMgr.move(camMgr.moveForward); // Move forward
            break;

        case OIS::KC_DOWN:
        case OIS::KC_S:
            mMc->directionKeyPressed = true;
            camMgr.move(camMgr.moveBackward); // Move backward
            break;

        case OIS::KC_PGUP:
        case OIS::KC_E:
            camMgr.move(camMgr.moveDown); // Move down
            break;

        case OIS::KC_INSERT:
        case OIS::KC_Q:
            camMgr.move(camMgr.moveUp); // Move up
            break;

        case OIS::KC_HOME:
            camMgr.move(camMgr.rotateUp); // Tilt up
            break;

        case OIS::KC_END:
            camMgr.move(camMgr.rotateDown); // Tilt down
            break;

        case OIS::KC_DELETE:
            camMgr.move(camMgr.rotateLeft); // Turn left
            break;

        case OIS::KC_PGDOWN:
            camMgr.move(camMgr.rotateRight); // Turn right
            break;

        //Toggle mCurrentTileType
        case OIS::KC_R:
        {
            mCurrentTileType = Tile::nextTileType((Tile::TileType)mCurrentTileType);
            std::stringstream tempSS("");
            tempSS << "Tile type:  " << Tile::tileTypeToString((Tile::TileType)mCurrentTileType);
            ODApplication::MOTD = tempSS.str();
        }
            break;

        //Decrease brush radius
        case OIS::KC_COMMA:
            if (mCurrentTileRadius > 1)
                --mCurrentTileRadius;

            ODApplication::MOTD = "Brush size:  "
                                    + Ogre::StringConverter::toString(mCurrentTileRadius);
            break;

        //Increase brush radius
        case OIS::KC_PERIOD:
            if (mCurrentTileRadius < 10)
                ++mCurrentTileRadius;

            ODApplication::MOTD = "Brush size:  "
                                    + Ogre::StringConverter::toString(mCurrentTileRadius);
            break;

        //Toggle mBrushMode
        case OIS::KC_B:
            mBrushMode = !mBrushMode;
            ODApplication::MOTD = (mBrushMode)
                                    ? "Brush mode turned on"
                                    : "Brush mode turned off";
            break;

        //Toggle mCurrentFullness
        case OIS::KC_T:
            mCurrentFullness = Tile::nextTileFullness(mCurrentFullness);
            ODApplication::MOTD = "Tile fullness:  "
                                  + Ogre::StringConverter::toString(mCurrentFullness);
            break;

        // Quit the Editor Mode
        case OIS::KC_ESCAPE:
            //MapLoader::writeGameMapToFile(std::string("levels/Test.level") + ".out", *mMc->gameMap);
            //mMc->frameListener->requestExit();
            progressMode(ModeManager::MENU);
            Gui::getSingletonPtr()->switchGuiMode();
            break;

        // Print a screenshot
        case OIS::KC_SYSRQ:
            ResourceManager::getSingleton().takeScreenshot();
            break;

        case OIS::KC_1:
        case OIS::KC_2:
        case OIS::KC_3:
        case OIS::KC_4:
        case OIS::KC_5:
        case OIS::KC_6:
        case OIS::KC_7:
        case OIS::KC_8:
        case OIS::KC_9:
        case OIS::KC_0:
            handleHotkeys(arg.key);
            break;

        default:
            break;
        }

    return true;
}

bool EditorMode::keyReleased(const OIS::KeyEvent& arg)
{
    CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan) arg.key);

    if (mMc->frameListener->isTerminalActive())
        return true;

    CameraManager& camMgr = *(mMc->frameListener->cm);
    switch (arg.key)
    {
    case OIS::KC_LEFT:
    case OIS::KC_A:
        mMc->directionKeyPressed = false;
        camMgr.move(camMgr.stopLeft);
        break;

    case OIS::KC_RIGHT:
    case OIS::KC_D:
        mMc->directionKeyPressed = false;
        camMgr.move(camMgr.stopRight);
        break;

    case OIS::KC_UP:
    case OIS::KC_W:
        mMc->directionKeyPressed = false;
        camMgr.move(camMgr.stopForward);
        break;

    case OIS::KC_DOWN:
    case OIS::KC_S:
        mMc->directionKeyPressed = false;
        camMgr.move(camMgr.stopBackward);
        break;

    case OIS::KC_PGUP:
    case OIS::KC_E:
        camMgr.move(camMgr.stopDown);
        break;

    case OIS::KC_INSERT:
    case OIS::KC_Q:
        camMgr.move(camMgr.stopUp);
        break;

    case OIS::KC_HOME:
        camMgr.move(camMgr.stopRotUp);
        break;

    case OIS::KC_END:
        camMgr.move(camMgr.stopRotDown);
        break;

    case OIS::KC_DELETE:
        camMgr.move(camMgr.stopRotLeft);
        break;

    case OIS::KC_PGDOWN:
        camMgr.move(camMgr.stopRotRight);
        break;

    default:
        break;
    }

    return true;
}

void EditorMode::handleHotkeys(OIS::KeyCode keycode)
{
    //keycode minus two because the codes are shifted by two against the actual number
    unsigned int keynumber = keycode - 2;

    if (mMc->mKeyboard->isModifierDown(OIS::Keyboard::Shift))
    {
        mMc->hotkeyLocationIsValid[keynumber] = true;
        mMc->hotkeyLocation[keynumber] = mMc->frameListener->cm->getCameraViewTarget();
    }
    else
    {
        if (mMc->hotkeyLocationIsValid[keynumber])
            mMc->frameListener->cm->flyTo(mMc->hotkeyLocation[keynumber]);
    }
}

bool EditorMode::isInGame()
{
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}

void EditorMode::giveFocus()
{
    mMc->mMouse->setEventCallback(this);
    mMc->mKeyboard->setEventCallback(this);
}
