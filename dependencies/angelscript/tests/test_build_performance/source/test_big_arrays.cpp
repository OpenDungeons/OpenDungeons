//
// Test author: Andreas Jonsson
//

#include "utils.h"
#include "memory_stream.h"

#include <string>
#include <sstream>
using std::string;
using std::stringstream;

namespace TestBigArrays
{

#define TESTNAME "TestBigArray"

void print( const string & )
{
}

void Test()
{
	printf("---------------------------------------------\n");
	printf("%s\n\n", TESTNAME);
	printf("AngelScript 2.25.1 WIP 0: 1.59 secs\n");
	printf("AngelScript 2.25.1 WIP 1: 1.66 secs (local bytecode optimizations)\n");
	printf("AngelScript 2.25.1 WIP 2: 1.60 secs (reversed order)\n");

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL);

	////////////////////////////////////////////
	printf("\nGenerating...\n");

	const int numArrays = 2;
	const int numElements = 100000;

	string script;
    std::stringstream script_buffer;
    for (unsigned i = 0; i < numArrays; i++)
    {
        script_buffer << "int[] array_" << i << " = {";
        if (numElements > 0)
        {
            script_buffer << 0;
        }
        for (unsigned j = 1; j < numElements; j++)
        {
            script_buffer << ", " << j;
        }
        script_buffer << "};";
    }
    script_buffer << std::endl << "int main() { print (\"elem 999 = \" + array_0[999] + \"\\n\"); return 0; }";
    script = script_buffer.str();

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



