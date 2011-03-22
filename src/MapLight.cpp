#include <sstream>

#include "MapLight.h"
#include "Globals.h"
#include "Functions.h"
#include "RenderRequest.h"
#include "GameMap.h"

void MapLight::initialize()
{
    static unsigned int lightNumber = 1;

    ogreEntityExists = false;

    std::stringstream tempSS;
    sem_wait(&lightNumberLockSemaphore);
    tempSS << "Map_light_ " << lightNumber++;
    sem_post(&lightNumberLockSemaphore);
    name = tempSS.str();

    thetaX = 0.0;
    thetaY = 0.0;
    thetaZ = 0.0;
    factorX = 1;
    factorY = 1;
    factorZ = 1;
}

MapLight::MapLight()
{
    initialize();
}

MapLight::MapLight(Ogre::Vector3 nPosition, Ogre::Real red, Ogre::Real green,
        Ogre::Real blue, Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
        Ogre::Real quadratic)
{
    initialize();

    setPosition(nPosition);
    setDiffuseColor(red, green, blue);
    setSpecularColor(red, green, blue);
    setAttenuation(range, constant, linear, quadratic);
}

void MapLight::setLocation(Ogre::Vector3 nPosition)
{
    //TODO: This needs to make a renderRequest to actually move the light.
    position = nPosition;
}

void MapLight::setDiffuseColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue)
{
    diffuseColor = Ogre::ColourValue(red, green, blue);
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::setSpecularColor(Ogre::Real red, Ogre::Real green, Ogre::Real blue)
{
    specularColor = Ogre::ColourValue(red, green, blue);
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::setAttenuation(Ogre::Real range, Ogre::Real constant, Ogre::Real linear,
        Ogre::Real quadratic)
{
    attenuationRange = range;
    attenuationConstant = constant;
    attenuationLinear = linear;
    attenuationQuadratic = quadratic;
    //TODO: Call refresh of the OGRE entity.
}

void MapLight::createOgreEntity()
{
    if (ogreEntityExists)
        return;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createMapLight;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);

    ogreEntityExists = true;
    ogreEntityVisualIndicatorExists = true;
}

void MapLight::destroyOgreEntity()
{
    if (!ogreEntityExists)
        return;

    destroyOgreEntityVisualIndicator();

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyMapLight;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);

    ogreEntityExists = false;
}

void MapLight::destroyOgreEntityVisualIndicator()
{
    if (!ogreEntityExists || !ogreEntityVisualIndicatorExists)
        return;

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyMapLightVisualIndicator;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);

    ogreEntityVisualIndicatorExists = false;
}

void MapLight::deleteYourself()
{
    destroyOgreEntity();

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::deleteMapLight;
    request->p = this;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

std::string MapLight::getName()
{
    return name;
}

void MapLight::setPosition(Ogre::Real nX, Ogre::Real nY, Ogre::Real nZ)
{
    Ogre::Vector3 tempPosition(nX, nY, nZ);
    setPosition(tempPosition);
}

void MapLight::setPosition(Ogre::Vector3 nPosition)
{
    position = nPosition;

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = std::string("MapLight_") + name + "_node";
    request->vec = position;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

Ogre::Vector3 MapLight::getPosition()
{
    return position;
}

Ogre::ColourValue MapLight::getDiffuseColor()
{
    return diffuseColor;
}

Ogre::ColourValue MapLight::getSpecularColor()
{
    return specularColor;
}

Ogre::Real MapLight::getAttenuationRange()
{
    return attenuationRange;
}

Ogre::Real MapLight::getAttenuationConstant()
{
    return attenuationConstant;
}

Ogre::Real MapLight::getAttenuationLinear()
{
    return attenuationLinear;
}

Ogre::Real MapLight::getAttenuationQuadratic()
{
    return attenuationQuadratic;
}

/** \brief Moves the light in a semi-random fashion around its "native" position.  The time
 * variable indicates how much time has elapsed since the last update.
 *
 */
void MapLight::advanceFlicker(Ogre::Real time)
{

    thetaX += factorX * 3.14 * time;
    thetaY += factorY * 3.14 * time;
    thetaZ += factorZ * 3.14 * time;

    if (randomDouble(0.0, 1.0) < 0.1)
        factorX *= -1;
    if (randomDouble(0.0, 1.0) < 0.1)
        factorY *= -1;
    if (randomDouble(0.0, 1.0) < 0.1)
        factorZ *= -1;

    flickerPosition = Ogre::Vector3(sin(thetaX), sin(thetaY), sin(thetaZ));

    // Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::moveSceneNode;
    request->str = std::string("MapLight_") + name + "_flicker_node";
    request->vec = flickerPosition;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    queueRenderRequest(request);
}

bool MapLight::isPermanent()
{
    return true;
}

std::string MapLight::getFormat()
{
    return "posX\tposY\tposZ\tdiffuseR\tdiffuseG\tdiffuseB\tspecularR\tspecularG\tspecularB\tattenRange\tattenConst\tattenLin\tattenQuad";
}

std::ostream& operator<<(std::ostream& os, MapLight *m)
{
    os << m->position.x << "\t" << m->position.y << "\t" << m->position.z
            << "\t";
    os << m->diffuseColor.r << "\t" << m->diffuseColor.g << "\t"
            << m->diffuseColor.b << "\t";
    os << m->specularColor.r << "\t" << m->specularColor.g << "\t"
            << m->specularColor.b << "\t";
    os << m->attenuationRange << "\t" << m->attenuationConstant << "\t";
    os << m->attenuationLinear << "\t" << m->attenuationQuadratic;

    return os;
}

std::istream& operator>>(std::istream& is, MapLight *m)
{
    is >> m->position.x >> m->position.y >> m->position.z;
    is >> m->diffuseColor.r >> m->diffuseColor.g >> m->diffuseColor.b;
    is >> m->specularColor.r >> m->specularColor.g >> m->specularColor.b;
    is >> m->attenuationRange >> m->attenuationConstant;
    is >> m->attenuationLinear >> m->attenuationQuadratic;

    return is;
}

TemporaryMapLight::TemporaryMapLight(Ogre::Vector3 nPosition, Ogre::Real red,
        Ogre::Real green, Ogre::Real blue, Ogre::Real range, Ogre::Real constant,
        Ogre::Real linear, Ogre::Real quadratic) :
    MapLight(nPosition, red, green, blue, range, constant, linear, quadratic)
{
    turnsUntilDestroyed = turnsUntilDestroyed = 2;
}

bool TemporaryMapLight::isPermanent()
{
    return false;
}

bool TemporaryMapLight::doUpkeep()
{
    if (--turnsUntilDestroyed <= 0)
    {
        // Remove this light from the game map since it no longer will exist.
        gameMap.removeMapLight(this);

        destroyOgreEntity();
        return false;
    }
    else
    {
        return true;
    }
}

