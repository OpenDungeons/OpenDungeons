//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "memory_stream.h"
#include <string>
#include <sstream>
using std::string;
using std::stringstream;

namespace TestManySymbols
{

#define TESTNAME "TestManySymbols"

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.25.1 WIP 0: 0.34 secs\n");
	printf("AngelScript 2.25.1 WIP 1: 0.33 secs (local bytecode optimizations)\n");
	printf("AngelScript 2.25.1 WIP 2: 0.32 secs (reversed order)\n");


 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);

	////////////////////////////////////////////
	printf("\nGenerating...\n");

	const int numSymbols = 10000;

	string script;
	script.reserve(numSymbols * 30);

    for( int i = 0; i < numSymbols; i++ )
    {
		char buf[500];
		sprintf(buf, "const int const_%d = %d;\n", i, i);
		script += buf;
    }

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



