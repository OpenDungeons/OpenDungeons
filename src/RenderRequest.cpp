#include "RenderRequest.h"

RenderRequest::RenderRequest() :
        type(noRequest),
        p(NULL),
        p2(NULL),
        p3(NULL),
        str(""),
        vec(Ogre::Vector3(0.0, 0.0, 0.0)),
        turnNumber(0),
        b(false)
{
}
