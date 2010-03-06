#include <sstream>
using namespace std;

#include "Globals.h"
#include "Functions.h"
#include "MapLight.h"
#include "RenderRequest.h"

MapLight::MapLight()
{
	static unsigned int lightNumber = 1;

	ogreEntityExists = false;

	stringstream tempSS;
	tempSS << "Map_light_ " << lightNumber;
	name = tempSS.str();

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
	//if(ogreEntityExists)
		//return;

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createMapLight;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	//FIXME:  Wait until the render queue has finished processing this request to move on.
	//sem_wait(&meshCreationFinishedSemaphore);
	ogreEntityExists = true;
}

void MapLight::destroyOgreEntity()
{
	//if(!ogreEntityExists)
		//return;

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyMapLight;
	request->p = this;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	ogreEntityExists = false;
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

