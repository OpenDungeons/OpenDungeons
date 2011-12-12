#include "gameobj.h"
#include "scriptmgr.h"
#include "gamemgr.h"

using namespace std;

CGameObj::CGameObj(char dispChar, int x, int y)
{
	isDead           = false;
	displayCharacter = dispChar;
	this->x          = x;
	this->y          = y;
	link             = new CGameObjLink(this);
	controller       = 0;
}

CGameObj::~CGameObj()
{
	if( link )
	{
		// Sever the link
		link->obj = 0;
		link->Release();
	}

	if( controller )
		controller->Release();
}

void CGameObj::OnThink()
{
	// Call the script controller's OnThink method
	if( controller )
		scriptMgr->CallOnThink(controller);
}

bool CGameObj::Move(int dx, int dy)
{
	// Check if it is actually possible to move to the desired position
	int x2 = x + dx;
	if( x2 < 0 || x2 > 9 ) return false;

	int y2 = y + dy;
	if( y2 < 0 || y2 > 9 ) return false;

	// Check with the game manager if another object isn't occupying this spot
	CGameObj *obj = gameMgr->GetGameObjAt(x2, y2);
	if( obj ) return false;

	// Now we can make the move
	x = x2;
	y = y2;

	return true;
}

void CGameObj::Send(asIScriptObject *msg, CGameObjLink *other)
{
	if( other && other->obj && other->obj->controller )
		scriptMgr->CallOnMessage(other->obj->controller, msg, link);
}
