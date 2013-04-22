//
// Tests the ternary operator ?:
//
// Author: Andreas Jonsson
//

#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"

static const char * const TESTNAME = "TestCondition";

using std::string;
static CScriptString *a = 0;

static const char *script1 =
"void Test(string strA, string strB)   \n"
"{                                     \n"
"  a = true ? strA : strB;             \n"
"  a = false ? \"t\" : \"f\";          \n"
"  SetAttrib(true ? strA : strB);      \n"
"  SetAttrib(false ? \"t\" : \"f\");   \n"
"}                                     \n"
"void SetAttrib(string str) {}         \n";
/*
static const char *script2 =
"void Test()                     \n"
"{                               \n"
"  int a = 0;                    \n"
"  Data *v = 0;                  \n"
"  Data *p;                      \n"
"  p = a != 0 ? v : 0;           \n"
"  p = a == 0 ? 0 : v;           \n"
"}                               \n";
*/
static const char *script3 =
"void Test()                                  \n"
"{                                            \n"
"  int test = 5;                              \n"
"  int test2 = int((test == 5) ? 23 : 12);    \n"
"}                                            \n";

static void formatf(asIScriptGeneric *gen)
{
	float f = gen->GetArgFloat(0);
	char buffer[25];
	sprintf(buffer, "%f", f);
	gen->SetReturnAddress(new CScriptString(buffer));
}

static void formatUI(asIScriptGeneric *gen)
{
	asUINT ui = gen->GetArgDWord(0);
	char buffer[25];
	sprintf(buffer, "%d", ui);
	gen->SetReturnAddress(new CScriptString(buffer));
}

static void print(asIScriptGeneric *gen)
{
	CScriptString *str = (CScriptString*)gen->GetArgObject(0);
	UNUSED_VAR(str);
//	printf((str + "\n").c_str());
}

bool TestCondition()
{
	bool fail = false;
	int r;

	a = new CScriptString();

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalProperty("string a", a);

//	engine->RegisterObjectType("Data", 0, asOBJ_REF | asOBJ_NOHANDLE);

	engine->RegisterGlobalFunction("string@ format(float)", asFUNCTION(formatf), asCALL_GENERIC);
	engine->RegisterGlobalFunction("string@ format(uint)", asFUNCTION(formatUI), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void print(string &in)", asFUNCTION(print), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = ExecuteString(engine, "print(a == \"a\" ? \"t\" : \"f\")");
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	mod->Build();

	r = ExecuteString(engine, "Test(\"t\", \"f\")", mod);
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

/*	mod->AddScriptSection(0, TESTNAME, script2, strlen(script2), 0);
	mod->Build(0);

	r = ExecuteString(engine, "Test()");
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}
*/
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	mod->Build();

	r = ExecuteString(engine, "Test()", mod);
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	r = ExecuteString(engine, "bool b = true; print(\"Test: \" + format(float(b ? 15 : 0)));");
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	r = ExecuteString(engine, "bool b = true; print(\"Test: \" + format(b ? 15 : 0));");
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	r = ExecuteString(engine, "(true) ? print(\"true\") : print(\"false\")");
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: ExecuteString() failed\n", TESTNAME);
	}

	const char *script = "double get_gameTime() { return 100; } \n"
				         "void advance(bool full) { \n"
                         "  nextThink = gameTime + ( 30.0 * (full ? 10.0 : 1.0) ); \n"
						 "} \n"
						 "double nextThink; \n";
	mod->AddScriptSection("script", script);
	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	r = ExecuteString(engine, "nextThink = 0; advance(true); assert( nextThink == 100 + 300 );", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	r = ExecuteString(engine, "nextThink = 0; advance(false); assert( nextThink == 100 + 30 );", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();
	a->Release();

	{
		CBufferedOutStream bout;
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		const char *script = "class T \n"
		                     "{ \n"
			                 "  T@ Get() \n"
		                     "  { \n"
							 "    T@ r; \n"
							 "    return (false ? null : r); \n"
	                         "  } \n"
	                         "} \n";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Having classes with the same name as the template subtype was causing problems
		script = "class T \n"
		    "{ \n"
			"  array<T@> Ts;\n"
			"  T@ Get(uint n)\n"
		    "  {\n"
			"    return (n>=Ts.length()) ? null : Ts[n];  \n"
			"  } \n"
			"} \n";

		bout.buffer = "";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Success
	return fail;
}
