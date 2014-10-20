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

#ifndef RENDERREQUEST_H
#define RENDERREQUEST_H

#include <string>
#include <OgreVector3.h>
#include <OgreQuaternion.h>

/*! \brief A data structure to be used for requesting that the OGRE rendering thread perform certain tasks.
 *
 *  This data structure is used filled out with a request and then placed in
 *  the global renderQueue.  The requests are taken out of the queue and
 *  processed by the frameStarted event in the ExampleFrameListener class.
 */
class RenderRequest
{
public:
    enum RequestType
    {
        createTile = 0,
        refreshTile,
        destroyTile,
        temporalMarkTile,
        attachTile,
        detachTile,
        attachEntity,
        detachEntity,
        toggleCreatureVisibility,
        showSquareSelector,
        createCreature,
        destroyCreature,
        setObjectAnimationState,
        createCreatureVisualDebug,
        destroyCreatureVisualDebug,
        createWeapon,
        destroyWeapon,
        pickUpEntity,
        dropHand,
        rotateHand,
        createRoom,
        destroyRoom,
        createRenderedMovableEntity,
        destroyRenderedMovableEntity,
        createTrap,
        destroyTrap,
        createMapLight,
        updateMapLight,
        destroyMapLight,
        destroyMapLightVisualIndicator,
        moveSceneNode,
        orientSceneNodeToward,
        reorientSceneNode,
        scaleSceneNode,
        noRequest
    };

    RenderRequest();

    long int turnNumber;
    RequestType type;
    //TODO - can we use a union here instead?
    void *p;
    void *p2;
    void *p3;
    std::string str;
    std::string str2;
    Ogre::Vector3 vec;
    Ogre::Quaternion quaternion;
    bool b;
};

#endif // RENDERREQUEST_H
