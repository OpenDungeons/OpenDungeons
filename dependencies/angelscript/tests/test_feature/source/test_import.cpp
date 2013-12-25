//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include "utils.h"

namespace TestImport
{

static const char * const TESTNAME = "TestImport";




static const char *script1 =
"import string Test(string s) from \"DynamicModule\";   \n"
"void main()                                            \n"
"{                                                      \n"
"  Test(\"test\");                                      \n"
"}                                                      \n";

static const char *script2 =
"string Test(string s)    \n"
"{                        \n"
"  number = 1234567890;   \n"
"  return \"blah\";       \n"
"}                        \n";


static const char *script3 =
"class A                                         \n"
"{                                               \n"
"  int a;                                        \n"
"}                                               \n"
"import void Func(A&out) from \"DynamicModule\"; \n"
"import A@ Func2() from \"DynamicModule\";       \n";


static const char *script4 = 
"class A                   \n"
"{                         \n"
"  int a;                  \n"
"}                         \n"
"void Func(A&out) {}       \n"
"A@ Func2() {return null;} \n";



bool Test()
{
	bool fail = false;

	int number = 0;
	int r;
	asIScriptEngine *engine = 0;
	COutStream out;

	// Test 1
	// Importing a function from another module
 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString_Generic(engine);
	engine->RegisterGlobalProperty("int number", &number);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":1", script1, strlen(script1), 0);
	mod->Build();

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":2", script2, strlen(script2), 0);
	mod->Build();

	// Bind all functions that the module imports
	r = engine->GetModule(0)->BindAllImportedFunctions(); assert( r >= 0 );

	ExecuteString(engine, "main()", engine->GetModule(0));

	engine->Release();

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		TEST_FAILED;
	}

	// Test 2
	// Two identical structures declared in different modules are not the same
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptString_Generic(engine);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":3", script3, strlen(script3), 0);
	r = mod->Build(); assert( r >= 0 );

	mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
	mod->AddScriptSection(":4", script4, strlen(script4), 0);
	r = mod->Build(); assert( r >= 0 );

	// Bind all functions that the module imports
	r = engine->GetModule(0)->BindAllImportedFunctions(); assert( r < 0 );

	{
		const char *script = 
		    "import int test(void) from 'mod1'; \n"
		    "void main() \n"
	   	    "{ \n"
		    "  int str; \n"
		    "  str = test(); \n"
		    "}\n";

		mod->AddScriptSection("4", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
	}

	engine->Release();

	// Test building the same script twice, where both imports from another pre-existing module
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		mod = engine->GetModule("m0", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script1", "shared interface I3{};void F(I3@){}");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("m1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script2", "interface I1{void f(I2@);};interface I2{};shared interface I3{};import void F(I3@) from 'm0';");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		r = mod->BindAllImportedFunctions();
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("m2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script2", "interface I1{void f(I2@);};interface I2{};shared interface I3{};import void F(I3@) from 'm0';");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		r = mod->BindAllImportedFunctions();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test import with global property accessors
	{
		CBufferedOutStream bout;
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("t", 
			"interface ITest {}; \n"
			"import ITest@ get_globalAccessor() from 'any_module'; \n"
			"class Crash : ITest { \n"
			"  ITest@ ptr; \n"
			"  Crash() { \n"
			"    @ptr = globalAccessor; \n"
			"  }\n"
			"}\n");
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

	// Test import with array
	// http://www.gamedev.net/topic/616554-bindallimportedfunctions-fail/page__gopid__4892532#entry4892532
	{
		CBufferedOutStream bout;
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("t", 
			"shared interface test1\n"
			"{\n"
			"  void foo(test2@[] a);\n"
			"}\n"
			"shared interface test2\n"
			"{\n"
			"}\n"
			"test1@ func()\n"
			"{\n"
			"  return null;\n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("t",
			"shared interface test1\n"
			"{\n"
			"  void foo(test2@[] a);\n"
			"}\n"
			"shared interface test2\n"
			"{\n"
			"}\n"
			"import test1@ func() from '1';\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = mod->BindAllImportedFunctions();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test import with default arguments
	// http://www.gamedev.net/topic/634184-crash-on-import-default-argument/
	{
		CBufferedOutStream bout;
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("t", 
			"import void crashMyAS( int x = 0 ) from 'other_module';\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		mod = engine->GetModule("other_module", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("t",
			"void crashMyAS(int) {}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = mod->BindAllImportedFunctions();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test problem with default arg in imported function
	// http://www.gamedev.net/topic/639247-import-function-with-default-argument/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("mod1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"import void test2( int expect, int x = -1 ) from 'mod2'; \n"
			"void test( int expect, int x = -1 ) \n"
			"{ \n"
			"	assert( expect == x  ); \n"
			"} \n"
			"void check() \n"
			"{ \n"
			"	test( -1 ); \n"
			"	test( -2, -2 ); \n"
			"	test( 2, 2 ); \n"
			"	test2( -1 ); \n"
			"	test2( -2, -2 ); \n"
			"	test2( 2, 2 ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void test2( int expect, int x = -1 ) \n"
			"{ \n"
			"	assert( expect == x ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("mod1");
		mod->BindAllImportedFunctions();
		r = ExecuteString(engine, "check()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace

