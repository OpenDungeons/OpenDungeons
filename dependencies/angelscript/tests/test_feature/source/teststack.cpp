// 
// Test designed to verify functionality of the dynamically growing stack
//
// Author: Andreas Jönsson
//

#include "utils.h"

static const char * const TESTNAME = "TestStack";


static const char *script =
"void recursive(int n) \n"  // 1
"{                     \n"  // 2
"  if( n > 0 )         \n"  // 3
"    recursive(n - 1); \n"  // 4
"}                     \n"; // 5

bool TestStack()
{
	bool fail = false;
	COutStream out;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script);
	int r = mod->Build();
	if( r < 0 )
	{
		printf("%s: Failed to build script\n", TESTNAME);
		TEST_FAILED;
	}

	asIScriptContext *ctx = engine->CreateContext();
	engine->SetEngineProperty(asEP_MAX_STACK_SIZE, 32); // 32 byte limit
	ctx->Prepare(engine->GetModule(0)->GetFunctionByDecl("void recursive(int)"));
	ctx->SetArgDWord(0, 100);
	r = ctx->Execute();
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: Execution didn't throw an exception as was expected\n", TESTNAME);
		TEST_FAILED;
	}

	ctx->Release();
	engine->Release();

	return fail;
}
