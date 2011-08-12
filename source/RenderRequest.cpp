#include "RenderRequest.h"

RenderRequest::RenderRequest() :
        turnNumber(0),
        type(noRequest),
        p(NULL),
        p2(NULL),
        p3(NULL),
        str(""),
        vec(Ogre::Vector3(0.0, 0.0, 0.0)),
        b(false)
{
}
