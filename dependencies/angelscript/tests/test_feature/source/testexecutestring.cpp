//
// Tests ExecuteString() with multiple lines of code
//
// Test author: Andreas Jonsson
//

#include "utils.h"

static const char * const TESTNAME = "TestExecuteString";

struct Obj
{
	bool a;
	bool b;
} g_Obj;


bool TestExecuteString()
{
	bool fail = false;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->RegisterObjectType("Obj", sizeof(Obj), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS);
	engine->RegisterObjectProperty("Obj", "bool a", asOFFSET(Obj,a));
	engine->RegisterObjectProperty("Obj", "bool b", asOFFSET(Obj,b));

	engine->RegisterGlobalProperty("Obj g_Obj", &g_Obj);

	g_Obj.a = false;
	g_Obj.b = true;

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	ExecuteString(engine, "g_Obj.a = true;\n"
		                  "g_Obj.b = false;\n");

	engine->Release();

	if( !g_Obj.a || g_Obj.b )
	{
		printf("%s: ExecuteString() didn't execute correctly\n", TESTNAME);
		TEST_FAILED;
	}
	
	// Success
	return fail;
}
