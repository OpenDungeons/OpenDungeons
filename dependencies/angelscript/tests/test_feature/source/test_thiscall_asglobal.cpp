//
// This test was designed to test the asOBJ_CLASS flag with THISCALL
//
// Author: Andreas Jonsson
//

#include "utils.h"

namespace TestThiscallAsGlobal
{

static const char * const TESTNAME = "TestThiscallAsGlobal";

class Class1
{
public:
	void TestMe(asDWORD newValue) { a = newValue; }
	asDWORD a;
};

static Class1 c1;

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	engine->RegisterGlobalFunction("void TestMe(uint val)", asMETHOD(Class1, TestMe), asCALL_THISCALL_ASGLOBAL, &c1);

	c1.a = 0;


	int r = ExecuteString(engine, "TestMe(0xDEADC0DE);");
	if( r < 0 )
	{
		printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
		TEST_FAILED;
	}

	if( c1.a != 0xDEADC0DE )
	{
		printf("Class member wasn't updated correctly\n");
		TEST_FAILED;
	}

	// It must not be possible to register without the object pointer
	CBufferedOutStream bout;
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = engine->RegisterGlobalFunction("void Fail()", asMETHOD(Class1, TestMe), asCALL_THISCALL_ASGLOBAL, 0);
	if( r != asINVALID_ARG )
		TEST_FAILED;

	engine->Release();
	return fail;
}

} // namespace
