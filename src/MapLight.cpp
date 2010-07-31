#include <sstream>

#include "Globals.h"
#include "Functions.h"
#include "MapLight.h"
#include "RenderRequest.h"

MapLight::MapLight()
{
	static unsigned int lightNumber = 1;

	ogreEntityExists = false;

	std::stringstream tempSS;
	tempSS << "Map_light_ " << lightNumber;
	name = tempSS.str();

	thetaX = 0.0;
	thetaY = 0.0;
	thetaZ = 0.0;
	factorX = 1;
	factorY = 1;
	factorZ = 1;

	lightNumber++;
}

void MapLight::setLocation(Ogre::Vector3 nPosition)
{
	//TODO: This needs to make a renderRequest to actually move the light.
	position = nPosition;
}

void MapLight::setDiffuseColor(double red, double green, double blue)
{
	diffuseColor = Ogre::ColourValue(red, green, blue);
	//TODO: Call refresh of the OGRE entity.
}

void MapLight::setSpecularColor(double red, double green, double blue)
{
	specularColor = Ogre::ColourValue(red, green, blue);
	//TODO: Call refresh of the OGRE entity.
}

void MapLight::setAttenuation (double range, double constant, double linear, double quadratic)
{
	attenuationRange = range;
	attenuationConstant = constant;
	attenuationLinear = linear;
	attenuationQuadratic = quadratic;
	//TODO: Call refresh of the OGRE entity.
}

void MapLight::createOgreEntity()
{
	if(ogreEntityExists)
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
	if(!ogreEntityExists)
		return;

	destroyOgreEntityVisualIndicator();

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyMapLight;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	ogreEntityExists = false;
	ogreEntityVisualIndicatorExists = false;
}

void MapLight::destroyOgreEntityVisualIndicator()
{
	if(!ogreEntityExists || !ogreEntityVisualIndicatorExists)
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

string MapLight::getName()
{
	return name;
}

void MapLight::setPosition(double nX, double nY, double nZ)
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
	request->str = (string)"MapLight_" + name + "_node";
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

double MapLight::getAttenuationRange()
{
	return attenuationRange;
}

double MapLight::getAttenuationConstant()
{
	return attenuationConstant;
}

double MapLight::getAttenuationLinear()
{
	return attenuationLinear;
}

double MapLight::getAttenuationQuadratic()
{
	return attenuationQuadratic;
}

/** \brief Moves the light in a semi-random fashion around its "native" position.  The time
  * variable indicates how much time has elapsed since the last update.
  *
*/
void MapLight::advanceFlicker(double time)
{

	thetaX += factorX * 3.14*time;
	thetaY += factorY * 3.14*time;
	thetaZ += factorZ * 3.14*time;

	if(randomDouble(0.0, 1.0) < 0.1)  factorX *= -1;
	if(randomDouble(0.0, 1.0) < 0.1)  factorY *= -1;
	if(randomDouble(0.0, 1.0) < 0.1)  factorZ *= -1;

	flickerPosition = Ogre::Vector3(sin(thetaX), sin(thetaY), sin(thetaZ));

	// Create a RenderRequest to notify the render queue that the scene node for this creature needs to be moved.
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::moveSceneNode;
	request->str = (string)"MapLight_" + name + "_flicker_node";
	request->vec = flickerPosition;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

string MapLight::getFormat()
{
        return "posX\tposY\tposZ\tdiffuseR\tdiffuseG\tdiffuseB\tspecularR\tspecularG\tspecularB\tattenRange\tattenConst\tattenLin\tattenQuad";
}

ostream& operator<<(ostream& os, MapLight *m)
{
	os << m->position.x << "\t" << m->position.y << "\t" << m->position.z << "\t";
	os << m->diffuseColor.r << "\t" << m->diffuseColor.g << "\t" << m->diffuseColor.b << "\t";
	os << m->specularColor.r << "\t" << m->specularColor.g << "\t" << m->specularColor.b << "\t";
	os << m->attenuationRange << "\t" << m->attenuationConstant << "\t";
	os << m->attenuationLinear << "\t" << m->attenuationQuadratic;

	return os;
}

istream& operator>>(istream& is, MapLight *m)
{
	is >> m->position.x >> m->position.y >> m->position.z;
	is >> m->diffuseColor.r >> m->diffuseColor.g >> m->diffuseColor.b;
	is >> m->specularColor.r >> m->specularColor.g >> m->specularColor.b;
	is >> m->attenuationRange >> m->attenuationConstant;
	is >> m->attenuationLinear >> m->attenuationQuadratic;

	return is;
}

