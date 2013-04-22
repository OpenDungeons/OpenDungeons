//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include <string>
#include <sstream>
#include "memory_stream.h"
using std::string;
using std::stringstream;

namespace TestManyFuncs
{

#define TESTNAME "TestManyFuncs"

static const char *scriptBegin =
"void main()                                                 \n"
"{                                                           \n";

static const char *scriptFuncDecl = 
"void Func%d() {} \n";

static const char *scriptMiddle = 
"   Func%d();  \n";

static const char *scriptEnd =
"}                                                           \n";

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.25.1 WIP 0: 0.28 secs\n");
	printf("AngelScript 2.25.1 WIP 1: 0.28 secs (local bytecode optimizations)\n");
	printf("AngelScript 2.25.1 WIP 2: 0.27 secs (reversed order)\n");


 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);

	////////////////////////////////////////////
	printf("\nGenerating...\n");

	const int numFuncs = 2000;
	const int numCalls = 20000;

	string script;
	script.reserve(strlen(scriptBegin) + numFuncs*(strlen(scriptFuncDecl)+5) + numCalls*(strlen(scriptMiddle)+5) + strlen(scriptEnd));
	for( int a = 0; a < numFuncs; a++ )
	{
		char buf[500];
		sprintf(buf, scriptFuncDecl, a);
		script += buf;
	}
	script += scriptBegin;
	for( int n = 0; n < numCalls; n++ )
	{
		char buf[500];
		sprintf(buf, scriptMiddle, n%numFuncs);
		script += buf;
	}
	script += scriptEnd;

	////////////////////////////////////////////
	printf("\nBuilding...\n");

	double time = GetSystemTimer();

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script.c_str(), script.size(), 0);
	int r = mod->Build();

	time = GetSystemTimer() - time;

	if( r != 0 )
		printf("Build failed\n", TESTNAME);
	else
		printf("Time = %f secs\n", time);

	////////////////////////////////////////////
	printf("\nSaving...\n");

	time = GetSystemTimer();

	CBytecodeStream stream("");
	mod->SaveByteCode(&stream);

	time = GetSystemTimer() - time;
	printf("Time = %f secs\n", time);
	printf("Size = %d\n", int(stream.buffer.size()));

	////////////////////////////////////////////
	printf("\nLoading...\n");

	time = GetSystemTimer();

	asIScriptModule *mod2 = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod2->LoadByteCode(&stream);

	time = GetSystemTimer() - time;
	printf("Time = %f secs\n", time);

	engine->Release();
}

} // namespace



