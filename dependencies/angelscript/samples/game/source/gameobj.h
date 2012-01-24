#ifndef GAMEOBJ_H
#define GAMEOBJ_H

#include <string>
#include <angelscript.h>

class CGameObjLink;

class CGameObj
{
public:
	CGameObj(char dispChar, int x, int y);
	~CGameObj();

	// This event handler is called by the game manager each frame
	void OnThink();

	bool Move(int dx, int dy);
	void Send(asIScriptObject *msg, CGameObjLink *other);

	std::string name;
	CGameObjLink *link;
	int x, y;
	char displayCharacter;
	bool isDead;
	asIScriptObject *controller;

protected:
};

// The CGameObjLink is a thin wrapper on the game object
// that provides a weak link that can be severed at any
// time by the application without having to worry about
// who might be holding a reference to the object
class CGameObjLink
{
public:
	CGameObjLink(CGameObj *obj) { this->obj = obj; refCount = 1; }
	int AddRef() 
	{ 
		return ++refCount; 
	}
	int Release() 
	{ 
		if( --refCount == 0 ) 
		{ 
			delete this; 
			return 0; 
		} 
		return refCount; 
	}

	int GetX() { if( obj ) return obj->x; return 0; }
	int GetY() { if( obj ) return obj->y; return 0; }
	bool Move(int dx, int dy) { if( obj ) return obj->Move(dx,dy); return false; }
	void Kill() { if( obj ) obj->isDead = true; }
	void Send(asIScriptObject *msg, CGameObjLink *other) { if( obj ) obj->Send(msg, other); }

protected:
	friend class CGameObj;

	~CGameObjLink() {}
	CGameObj *obj;
	int refCount;
};


#endif