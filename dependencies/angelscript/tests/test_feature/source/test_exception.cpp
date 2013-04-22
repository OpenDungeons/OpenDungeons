#include "utils.h"
using namespace std;

static const char * const TESTNAME = "TestException";

// This script will cause an exception inside a class method
const char *script1 =
"class A               \n"
"{                     \n"
"  void Test(string c) \n"
"  {                   \n"
"    int a = 0, b = 0; \n"
"    a = a/b;          \n"
"  }                   \n"
"}                     \n";

static void print(asIScriptGeneric *gen)
{
	std::string *s = (std::string*)gen->GetArgAddress(0);
	UNUSED_VAR(s);
}

bool TestException()
{
	bool fail = false;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_GENERIC);


	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "int a = 0;\na = 10/a;", 0, ctx); // Throws an exception
	if( r == asEXECUTION_EXCEPTION )
	{
		int line = ctx->GetExceptionLineNumber();
		const char *desc = ctx->GetExceptionString();

		const asIScriptFunction *function = ctx->GetExceptionFunction();
		if( strcmp(function->GetName(), "ExecuteString") != 0 )
		{
			printf("%s: Exception function name is wrong\n", TESTNAME);
			TEST_FAILED;
		}
		if( strcmp(function->GetDeclaration(), "void ExecuteString()") != 0 )
		{
			printf("%s: Exception function declaration is wrong\n", TESTNAME);
			TEST_FAILED;
		}

		if( line != 2 )
		{
			printf("%s: Exception line number is wrong\n", TESTNAME);
			TEST_FAILED;
		}
		if( strcmp(desc, "Divide by zero") != 0 )
		{
			printf("%s: Exception string is wrong\n", TESTNAME);
			TEST_FAILED;
		}
	}
	else
	{
		printf("%s: Failed to raise exception\n", TESTNAME);
		TEST_FAILED;
	}

	ctx->Release();

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1));
	mod->Build();
	r = ExecuteString(engine, "A a; a.Test(\"test\");", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		TEST_FAILED;
	}

	// A test to validate Unprepare without execution
	{
		asIObjectType *type = mod->GetObjectTypeByIndex(0);
		asIScriptFunction *func = type->GetMethodByDecl("void Test(string c)");
		ctx = engine->CreateContext();
		ctx->Prepare(func);
		asIScriptContext *obj = (asIScriptContext*)engine->CreateScriptObject(type->GetTypeId());
		ctx->SetObject(obj); // Just sets the address
		CScriptString *str = new CScriptString();
		ctx->SetArgObject(0, str); // Makes a copy of the object
		str->Release();
		ctx->Unprepare(); // Must release the string argument, but not the object
		ctx->Release();
		obj->Release();
	}

	// Another test to validate Unprepare without execution
	{
		asIObjectType *type = mod->GetObjectTypeByIndex(0);
		// Get the real method, not the virtual method
		asIScriptFunction *func = type->GetMethodByDecl("void Test(string c)", false);
		ctx = engine->CreateContext();
		ctx->Prepare(func);
		// Don't set the object, nor the arguments
		ctx->Unprepare();
		ctx->Release();
	}

	// A test to verify behaviour when exception occurs in script class constructor
	const char *script2 = "class SomeClassA \n"
	"{ \n"
	"	int A; \n"
	" \n"
	"	~SomeClassA() \n"
	"	{ \n"
	"		print('destruct'); \n"
	"	} \n"
	"} \n"
	"class SomeClassB \n"
	"{ \n"
	"	SomeClassA@ nullptr; \n"
	"	SomeClassB(SomeClassA@ aPtr) \n"
	"	{ \n"
	"		this.nullptr.A=100; // Null pointer access. After this class a is destroyed. \n"
	"	} \n"
	"} \n"
	"void test() \n"
	"{ \n"
	"	SomeClassA a; \n"
	"	SomeClassB(a); \n"
	"} \n";
	mod->AddScriptSection("script2", script2);
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	r = ExecuteString(engine, "test()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		TEST_FAILED;
	}

	engine->GarbageCollect();

	engine->Release();

	// Problem reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test",
			"string post_score(string url, string channel, string channel_password, int score, string name, string email, string country) \n"
			"{ \n"
			"  string[] list={'something'}; \n"
			"  return list[1]; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  string result=post_score('hello', 'palacepunchup', 'anka', -1, 'Philip', 'philip@blastbay.com', 'Sweden'); \n"
			"}\n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( string(ctx->GetExceptionString()) != "Index out of bounds" )
			TEST_FAILED;
		if( string(ctx->GetExceptionFunction()->GetName()) != "post_score" )
			TEST_FAILED;

		ctx->Release();
		engine->Release();
	}

	// Test exception within default constructor
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test",
			"class Test { \n"
			"  string mem = 'hello'; \n"
			"  int a = 0; \n"
			"  int b = 10/a; \n"
			"}\n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "Test t;", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( string(ctx->GetExceptionString()) != "Divide by zero" )
		{
			printf("%s\n", ctx->GetExceptionString());
			TEST_FAILED;
		}
		if( string(ctx->GetExceptionFunction()->GetName()) != "Test" )
			TEST_FAILED;

		ctx->Release();
		engine->Release();
	}

	// Test exception in for-condition
	// http://www.gamedev.net/topic/638128-bug-with-show-code-line-after-null-pointer-exception-and-for/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class A\n"
			"{\n"
			"    int GetCount(){ return 10; }\n"
			"}\n"
			"void startGame()\n"
			"{\n"
			"    A @a = null;\n"
			"    for( int i=0; i < a.GetCount(); i++ )\n"
			"    {\n"
			"        int some_val;\n"
			"    }\n"
			"}\n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "startGame();", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( string(ctx->GetExceptionString()) != "Null pointer access" )
		{
			printf("%s\n", ctx->GetExceptionString());
			TEST_FAILED;
		}
		if( string(ctx->GetExceptionFunction()->GetName()) != "startGame" )
			TEST_FAILED;
		if( ctx->GetExceptionLineNumber() != 8 )
			TEST_FAILED;

		ctx->Release();
		engine->Release();
	}

	// Success
	return fail;
}
