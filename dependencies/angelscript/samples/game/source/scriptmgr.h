#ifndef SCRIPTMGR_H
#define SCRIPTMGR_H

#include <string>
#include <vector>
#include <angelscript.h>

class CGameObjLink;

class CScriptMgr
{
public:
	CScriptMgr();
	~CScriptMgr();

	int Init();

	asIScriptObject *CreateController(const std::string &type, CGameObjLink *link);
	void CallOnThink(asIScriptObject *object);
	void CallOnMessage(asIScriptObject *object, asIScriptObject *msg, CGameObjLink *link);

	bool hasCompileErrors;

protected:
	void MessageCallback(const asSMessageInfo &msg);
	asIScriptContext *PrepareContextFromPool(int funcId);
	void ReturnContextToPool(asIScriptContext *ctx);
	int ExecuteCall(asIScriptContext *ctx);

	struct SController
	{
		std::string module;
		asIObjectType *type;
		int factoryFuncId;
		int onThinkMethodId;
		int onMessageMethodId;
	};

	SController *GetControllerScript(const std::string &type);

	asIScriptEngine  *engine;

	// Our pool of script contexts. This is used to avoid allocating
	// the context objects all the time. The context objects are quite
	// heavy weight and should be shared between function calls.
	std::vector<asIScriptContext *> contexts;

	// This is the cache of function ids etc that we use to avoid having
	// to search for the function ids everytime we need to call a function.
	// The search is quite time consuming and should only be done once.
	std::vector<SController *> controllers; 
};

extern CScriptMgr *scriptMgr;

#endif