#include "utils.h"
#include "scriptmath3d.h"

namespace TestUnsafeRef
{

static const char * const TESTNAME = "TestUnsafeRef";

static const char *script1 =
"void Test()                            \n"
"{                                      \n"
"   int[] arr = {0};                    \n"
"   TestRefInt(arr[0]);                 \n"
"   Assert(arr[0] == 23);               \n"
"   int a = 0;                          \n"
"   TestRefInt(a);                      \n"
"   Assert(a == 23);                    \n"
"   string[] sa = {\"\"};               \n"
"   TestRefString(sa[0]);               \n"
"   Assert(sa[0] == \"ref\");           \n"
"   string s = \"\";                    \n"
"   TestRefString(s);                   \n"
"   Assert(s == \"ref\");               \n"
"}                                      \n"
"void TestRefInt(int &ref)              \n"
"{                                      \n"
"   ref = 23;                           \n"
"}                                      \n"
"void TestRefString(string &ref)        \n"
"{                                      \n"
"   ref = \"ref\";                      \n"
"}                                      \n";


struct Str
{
public:
	Str() {};
	Str(const Str &o) {str = o.str;}

	static void StringConstruct(Str *p) { new(p) Str(); }
	static void StringCopyConstruct(const Str &o, Str *p) { new(p) Str(o); }
	static void StringDestruct(Str *p) { p->~Str(); }
	static Str StringFactory(unsigned int length, const char *s) { Str str; str.str = s; return str; }
	bool opEquals(const Str &o) { return str == o.str; }
	Str &opAssign(const Str &o) { str = o.str; return *this; }

	std::string str;
};

bool Test()
{
	bool fail = false;
	int r;

	COutStream out;
	CBufferedOutStream bout;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, true);
	RegisterScriptString(engine);

	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "Test()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
		printf("%s: Execution failed: %d\n", TESTNAME, r);
	}

	if( ctx ) ctx->Release();

	engine->Release();

	// Test value class with unsafe ref
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"class Good \n"
			"{ \n"
			"  vector3 _val; \n"
			"  Good(const vector3& in val) \n"
			"  { \n"
			"    _val = val; \n"
			"  }  \n"
			"};  \n"
			"class Bad  \n"
			"{  \n"
			"  vector3 _val;  \n"
			"  Bad(const vector3& val)  \n"
			"  {  \n"
			"    _val = val;  \n"
			"  }  \n"
			"}; \n"
			"void test()  \n"
			"{ \n"
			"  // runs fine  \n"
			"  for (int i = 0; i < 2; i++)  \n"
			"    Good(vector3(1, 2, 3));  \n"
			"  // causes vm stack corruption  \n"
			"  for (int i = 0; i < 2; i++)  \n"
			"    Bad(vector3(1, 2, 3));  \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "test()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test ref to primitives
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"void func(){ \n"
			"  float a; \n"
			"  uint8 b; \n"
			"  int c; \n"
			"  funcA(c, a, b); \n"
			"} \n"
			"void funcA(float& a, uint8& b, int& c) {} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "TestUnsafeRef (1, 1) : Info    : Compiling void func()\n"
		                   "TestUnsafeRef (5, 3) : Error   : No matching signatures to 'funcA(int, float, uint8)'\n"
		                   "TestUnsafeRef (5, 3) : Info    : Candidates are:\n"
		                   "TestUnsafeRef (5, 3) : Info    : void funcA(float&inout, uint8&inout, int&inout)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test problem found by TheAtom
	// Passing an inout reference to a handle to a function wasn't working properly
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"class T { int a; } \n"
			"void f(T@& p) { \n"
			"  T t; \n"
			"  t.a = 42; \n"
			"  @p = t; \n" // or p=t; in which case t is copied
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "T @t; f(t); assert( t.a == 42 );\n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();		
	}

	// http://www.gamedev.net/topic/624722-bug-with/
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"class T { T() { val = 123; } int val; } \n"
			"T g_t; \n"
			"T &GetTest() { return g_t; } \n"
			"void f(T@& t) { \n"
			"  assert( t.val == 123 ); \n"
			"} \n"
			"void func() { \n"
			"  f(GetTest()); \n"
			"  f(@GetTest()); \n"
			"  T @t = GetTest(); \n"
			"  f(t); \n"
			"} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "TestUnsafeRef (7, 1) : Info    : Compiling void func()\n"
						   "TestUnsafeRef (8, 3) : Error   : No matching signatures to 'f(T)'\n"
						   "TestUnsafeRef (8, 3) : Info    : Candidates are:\n"
						   "TestUnsafeRef (8, 3) : Info    : void f(T@&inout)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();		
	}

	// http://www.gamedev.net/topic/624722-bug-with/
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"class T { T() { val = 123; } int val; } \n"
			"T g_t; \n"
			"T &GetTest() { return g_t; } \n"
			"void f(T@& t) { \n"
			"  assert( t.val == 123 ); \n"
			"} \n"
			"void func() { \n"
			"  f(cast<T>(GetTest())); \n"
			"  f(@GetTest()); \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "func()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
			if( r == asEXECUTION_EXCEPTION )
				PrintException(ctx, true);
		}
		ctx->Release();

		engine->Release();		
	}

	// http://www.gamedev.net/topic/636443-there-is-no-copy-operator-for-the-type-val-available/
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->RegisterObjectType("Val", sizeof(int), asOBJ_VALUE | asOBJ_APP_PRIMITIVE);
		engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(0), asCALL_GENERIC);
		// With unsafe references the copy constructor doesn't have to be in, it can be inout too
		engine->RegisterObjectBehaviour("Val", asBEHAVE_CONSTRUCT, "void f(const Val &)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(0), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"Val GetVal() \n"
			"{ \n"
			"    Val ret; \n"
			"    return ret; \n"
			"} \n");

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

	// Test with copy constructor that takes unsafe reference
	// http://www.gamedev.net/topic/638613-asassert-in-file-as-compillercpp-line-675/
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = engine->RegisterObjectType("string", sizeof(Str), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
		r = engine->RegisterStringFactory("string", asFUNCTION(Str::StringFactory), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Str::StringConstruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		// Copy constructor takes an unsafe reference
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string &)", asFUNCTION(Str::StringCopyConstruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Str::StringDestruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in)", asMETHOD(Str, opEquals), asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"void SetTexture( string txt ) { assert( txt == 'test' ); } \n"
			"void startGame( ) \n"
			"{ \n"
			"   SetTexture('test'); \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "startGame();", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test with assignment operator that takes unsafe reference
	{
		bout.buffer = "";
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = engine->RegisterObjectType("string", sizeof(Str), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
		r = engine->RegisterStringFactory("string", asFUNCTION(Str::StringFactory), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Str::StringConstruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Str::StringDestruct), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		// Assignment operator takes an unsafe reference
		r = engine->RegisterObjectMethod("string", "string &opAssign(const string &)", asMETHOD(Str, opAssign), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in)", asMETHOD(Str, opEquals), asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, 
			"void SetTexture( string txt ) { assert( txt == 'test' ); } \n"
			"void startGame( ) \n"
			"{ \n"
			"   SetTexture('test'); \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "startGame();", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Success
	return fail;
}

} // namespace
