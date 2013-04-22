#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"

namespace Test_Addon_ScriptArray
{

static const char *TESTNAME = "Test_Addon_ScriptArray";

static const char *script1 =
"array<string> b;                                \n"
"array<int> g_a(3);                              \n"
"void TestArray()                                \n"
"{                                               \n"
"   array<string> a(5);                          \n"
"   Assert(a.length() == 5);                     \n"
"   a.resize(10);                                \n"
"   a.resize(5);                                 \n"
"   a[0] = \"Hello\";                            \n"
"   Assert(a[0] == \"Hello\");                   \n"
"   uint n = 0;                                  \n"
"   Assert(a[n] == \"Hello\");                   \n"
"   n++;                                         \n"
"   Assert(a[n] == \"\");                        \n"
"   b = a;                                       \n"
"   Assert(b.length() == 5);                     \n"
"   Assert(b[0] == \"Hello\");                   \n"
"   b[0] = \"Goodbye\";                          \n"
"   Assert(a[0] != \"Goodbye\");                 \n"
"   array<int> ia = TestArray4();                \n"
"   TestArray2(ia);                              \n"
"   TestArray3(ia);                              \n"
"   ia = array<int>(3);                          \n"
"   Assert(ia.length() == 3);                    \n"
"   ia[0] = 1;                                   \n"
"   array<int> ib = ia;                          \n"
"   Assert(ib.length() == ia.length());          \n"
"   Assert(ib[0] == ia[0]);                      \n"
"}                                               \n"
"void TestArray2(array<int> &inout a)            \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"void TestArray3(array<int> a)                   \n"
"{                                               \n"
"   Assert(a[0] == 1);                           \n"
"   Assert(a[1] == 2);                           \n"
"   Assert(a[2] == 3);                           \n"
"}                                               \n"
"array<int> TestArray4()                         \n"
"{                                               \n"
"   array<int> ia(3);                            \n"
"   ia[0] = 1;                                   \n"
"   ia[1] = 2;                                   \n"
"   ia[2] = 3;                                   \n"
"   return ia;                                   \n"
"}                                               \n";

static const char *script2 = 
"void TestArrayException()                       \n"
"{                                               \n"
"   array<string> a;                             \n"
"   a[0] == \"Hello\";                           \n"
"}                                               \n";

// Must be possible to declare array of arrays
static const char *script3 = 
"void TestArrayMulti()                           \n"
"{                                               \n"
"   array<array<int>> a(2);                      \n"
"   array<int> b(2);                             \n"
"   a[0] = b;                                    \n"
"   a[1] = b;                                    \n"
"                                                \n"
"   a[0][0] = 0;                                 \n"
"   a[0][1] = 1;                                 \n"
"   a[1][0] = 2;                                 \n"
"   a[1][1] = 3;                                 \n"
"                                                \n"
"   Assert(a[0][0] == 0);                        \n"
"   Assert(a[0][1] == 1);                        \n"
"   Assert(a[1][0] == 2);                        \n"
"   Assert(a[1][1] == 3);                        \n"
"}                                               \n";

static const char *script4 = 
"void TestArrayChar()                            \n"
"{                                               \n"
"   array<int8> a(2);                            \n"
"   a[0] = 13;                                   \n"
"   a[1] = 19;                                   \n"
"                                                \n"
"   int8 a0 = a[0];                              \n"
"   int8 a1 = a[1];                              \n"
"   Assert(a[0] == 13);                          \n"
"   Assert(a[1] == 19);                          \n"
"}                                               \n";

static const char *script5 = 
"array<int> g = {1,2,3};                         \n"
"void TestArrayInitList()                        \n"
"{                                               \n"
"   Assert(g.length() == 3);                     \n"
"   Assert(g[2] == 3);                           \n"
"   array<int> a = {,2,};                        \n"
"   Assert(a.length() == 3);                     \n"
"   Assert(a[1] == 2);                           \n"
"   array<string> b = {\"test\", \"3\"};         \n"
"   Assert(b.length() == 2);                     \n"
"   Assert(b[0] == \"test\");                    \n"
"   Assert(b[1] == \"3\");                       \n"
"   array<array<int>> c = {,{23},{23,4},};       \n"
"   Assert(c.length() == 4);                     \n"
"   Assert(c[2].length() == 2);                  \n"
"   Assert(c[2][1] == 4);                        \n"
"   const array<int> d = {0,1,2};                \n"
"   Assert(d.length() == 3);                     \n"
"   Assert(d[2] == 2);                           \n"
"}                                               \n";

static const char *script6 =
"void Test()                                     \n"
"{                                               \n"
"   array<int>@ e = {2,5};                       \n"
"   array<int> f = {,{23}};                      \n"
"}                                               \n";

static const char *script7 =
"class TestC                                     \n"
"{                                               \n"
"  TestC() {count++; s = \"test\";}              \n"
"  string s;                                     \n"
"}                                               \n"
"int count = 0;                                  \n"
"void Test()                                     \n"
"{                                               \n"
"  TestC t;                                      \n"
"  Assert(count == 1);                           \n"
"  array<TestC> arr(5);                          \n"
"  Assert(count == 6);                           \n"
"}                                               \n";

bool Test2();

CScriptArray *CreateArrayOfStrings()
{
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
	{
		asIScriptEngine* engine = ctx->GetEngine();
		asIObjectType* t = engine->GetObjectTypeById(engine->GetTypeIdByDecl("array<string@>"));
		CScriptArray* arr = new CScriptArray(3, t);
		for( asUINT i = 0; i < arr->GetSize(); i++ )
		{
			CScriptString** p = static_cast<CScriptString**>(arr->At(i));
			*p = new CScriptString("test");
		}
		return arr;
	}
	return 0;
}

bool Test()
{
	bool fail = false;
	fail = Test2() || fail;
	int r;
	COutStream out;
	CBufferedOutStream bout;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);
	RegisterScriptArray(engine, false);

	engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);


	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArray()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		printf("%s: Failed to execute script\n", TESTNAME);
		TEST_FAILED;
	}
	if( ctx ) ctx->Release();

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	r = ExecuteString(engine, "TestArrayException()", mod);
	if( r != asEXECUTION_EXCEPTION )
	{
		printf("%s: No exception\n", TESTNAME);
		TEST_FAILED;
	}

	// Must be possible to declare array of arrays
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayMulti()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failure\n", TESTNAME);
		TEST_FAILED;
	}
	if( r == asEXECUTION_EXCEPTION )
	{
		PrintException(ctx);
	}
	if( ctx ) ctx->Release();
	ctx = 0;


	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}
	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayChar()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		printf("%s: Failure\n", TESTNAME);
		TEST_FAILED;
	}
	if( r == asEXECUTION_EXCEPTION )
	{
		PrintException(ctx);
	}

	if( ctx ) ctx->Release();

	// Initialization lists must work for array template
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestArrayInitList()", mod, ctx);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	if( r == asEXECUTION_EXCEPTION )
		PrintException(ctx);

	if( ctx ) ctx->Release();

	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "Test_Addon_ScriptArray (1, 1) : Info    : Compiling void Test()\n"
	                   "Test_Addon_ScriptArray (3, 20) : Error   : Initialization lists cannot be used with 'array<int>@'\n"
	                   "Test_Addon_ScriptArray (4, 21) : Error   : Initialization lists cannot be used with 'int'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Array object must call default constructor of the script classes
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
	r = mod->Build();
	if( r < 0 ) 
		TEST_FAILED;
	r = ExecuteString(engine, "Test()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
		
	// Test bool[] on Mac OS X with PPC CPU
	// Submitted by Edward Rudd
	const char *script8 =
	"array<bool> f(10);         \n"
	"for (int i=0; i<10; i++) { \n"
	"	f[i] = false;           \n"
	"}                          \n"
	"Assert(f[0] == false);     \n"
	"Assert(f[1] == false);     \n"
	"f[0] = true;               \n"
	"Assert(f[0] == true);      \n"
	"Assert(f[1] == false);     \n";
	
	r = ExecuteString(engine, script8, mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// Test reserve()
	{
		const char *script = 
			"array<int> f; \n"
			"f.reserve(10); \n"
			"for( uint n = 0; n < 10; n++ ) \n"
			"  f.insertAt(n, n); \n"
			"Assert( f.length() == 10 ); \n";
		r = ExecuteString(engine, script, mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	}

	// Make sure it is possible to do multiple assignments with the array type
	r = ExecuteString(engine, "array<int> a, b, c; a = b = c;");
	if( r < 0 )
		TEST_FAILED;

	// Must support syntax as: array<array<int>>, i.e. without white space between the closing angled brackets.
	r = ExecuteString(engine, "array<array<int>> a(2); Assert( a.length() == 2 );");
	if( r < 0 )
		TEST_FAILED;

	// Must support arrays of handles
	r = ExecuteString(engine, "array<array<int>@> a(1); @a[0] = @array<int>(4);");
	if( r < 0 )
		TEST_FAILED;

	// Do not allow the instantiation of a template with a subtype that cannot be created
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->RegisterObjectType("single", 0, asOBJ_REF | asOBJ_NOHANDLE);
	r = ExecuteString(engine, "array<single> a;");
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 7) : Error   : Can't instanciate template 'array' with subtype 'single'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	// Test too large arrays
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, false);

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "array<int> a; a.resize(0xFFFFFFFF);", 0, ctx);
		if( r != asEXECUTION_EXCEPTION )
		{
			TEST_FAILED;
		}
		else if( strcmp(ctx->GetExceptionString(), "Too large array size") != 0 )
		{
			TEST_FAILED;
		}

		r = ExecuteString(engine, "array<int> a(0xFFFFFFFF);", 0, ctx);
		if( r != asEXECUTION_EXCEPTION )
		{
			TEST_FAILED;
		}
		else if( strcmp(ctx->GetExceptionString(), "Too large array size") != 0 )
		{
			TEST_FAILED;
		}

		ctx->Release();
		engine->Release();
	}

	// Test garbage collect with script class that holds array member
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("module", asGM_ALWAYS_CREATE);

		const char *script = 
			"class MyTest \n"
			"{ \n"
			"	array<int> myList; \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("MyTest"));
		obj->Release();

		engine->Release();
	}

	// Test the default value constructor
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("module", asGM_ALWAYS_CREATE);
		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		const char *script = 
			"void main() \n"
			"{ \n"
			"	array<int> arr(2, 42); \n"
			"   assert(arr[0] == 42); \n"
			"   assert(arr[1] == 42); \n"
			"   array<array<int>> arr2(2, array<int>(2)); \n"
			"   assert(arr2[0].length() == 2); \n"
			"	assert(arr2[1].length() == 2); \n"
			"   array<array<int>@> arr3(2, arr); \n"
			"   assert(arr3[0] is arr); \n"
			"   assert(arr3[1] is arr); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test potential memory leak situation with circular reference between types
	{
		// Create the script engine
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	 
		// Register array class
		RegisterScriptArray(engine, false);
	 
		// Compile
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", 
			"class Hoge"
			"{"
			"    HogeManager@ hogeManager;"
			"};"
			"class HogeManager"
			"{"
			"    array< Hoge >@ hoges;"
			"};"
			, 0);
		mod->Build();
	 
		// Release engine
		engine->Release();
	}

	// Test creating script array from application
	{
		if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		{
			printf("Subtest: Skipped due to AS_MAX_PORTABILITY\n");
		}
		else
		{
			asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			RegisterScriptArray(engine, false);
			RegisterScriptString(engine);
		
			r = engine->RegisterGlobalFunction("array<string@>@ CreateArrayOfStrings()", asFUNCTION(CreateArrayOfStrings), asCALL_CDECL); assert( r >= 0 );

			r = ExecuteString(engine, "array<string@>@ arr = CreateArrayOfStrings()");
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
		 
			// Release engine
			engine->Release();
		}		
	}

	// Test insertAt, removeAt
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, false);
		RegisterScriptString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, "array<string> arr = {'1','2','3'}; \n"
			                      "arr.insertAt(1, 'test'); \n"
								  "assert( arr[1] == 'test' );\n"
								  "arr.insertAt(4, '4'); \n"
								  "assert( arr[4] == '4' );\n"
								  "arr.removeAt(0); \n"
								  "assert( arr[0] == 'test' );\n"
								  "assert( arr[3] == '4' );\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// This test was failing on XBOX 360
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterScriptArray(engine, true);
		
		const char *script = 
			"class ArrayOf  \n"
			"{  \n"
			"  uint8[] _boolList;  \n"
			"  int _numOfStockedObject;  \n"
			"  ArrayOf(int arraySizeMax)  \n"
			"  {  \n"
			"    _boolList.resize(arraySizeMax);  \n"
			"    _numOfStockedObject = 0;  \n"
			"    for(int i = 0; i < arraySizeMax; ++i)  \n"
			"    {  \n"
			"       _boolList[i] = 0; \n"
			"    } \n"
			"  } \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "ArrayOf(100)", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Array should call subtypes' opAssign when it exists
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"int calls = 0; \n"
			"class Value  \n"
			"{  \n"
			"  int val;  \n"
			"  Value(int v) {val = v;} \n"
			"  Value() {} \n"
			"  Value &opAssign(const Value &in o) { calls++; return this; } \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "array<Value> arr = {Value(2), Value(3), Value(0)}; \n"
								  "assert( calls == 3 ); \n"
								  "array<Value> arr2; \n"
								  "arr2 = arr; \n"
								  "assert( calls == 6 ); \n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// test sorting
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"class Value  \n"
			"{  \n"
			"  int val;  \n"
			"  Value(int v) {val = v;} \n"
			"  Value() {} \n"
			"  int opCmp(const Value &in o) {return val - o.val;} \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "Value[] arr = {Value(2), Value(3), Value(0)}; \n"
			                      "arr.sortAsc(); \n"
								  "assert(arr[0].val == 0); \n"
								  "assert(arr[1].val == 2); \n"
								  "assert(arr[2].val == 3);", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test 
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"bool TestSort() \n"
			"{ \n"
			"	array<int> A = {1, 5, 2, 4, 3}; \n"
			"	array<int> B = {1, 5, 2, 4, 3}; \n"
			"	A.sortAsc(); \n"
			"	B.sortDesc(); \n"
			"	return \n"
			"		A[0] == 1 && A[1] == 2 && A[2] == 3 && A[3] == 4 && A[4] == 5 && \n"
			"		B[0] == 5 && B[1] == 4 && B[2] == 3 && B[3] == 2 && B[4] == 1; \n"
			"} \n"
			"bool TestReverse() \n"
			"{ \n"
			"	array<int> A = {5, 4, 3, 2, 1}; \n"
			"	A.reverse(); \n"
			"	return A[0] == 1 && A[1] == 2 && A[2] == 3 && A[3] == 4 && A[4] == 5; \n"
			"} \n"
			"class cOpCmp \n"
			"{ \n"
			"	cOpCmp() \n"
			"	{ \n"
			"		a = 0; \n"
			"		b = 0.0; \n"
			"	}	 \n"
			"	cOpCmp(int _a, float _b) \n"
			"	{ \n"
			"		a = _a; \n"
			"		b = _b;	 \n"
			"	} \n"
			"	void set(int _a, float _b) \n"
			"	{ \n"
			"		a = _a; \n"
			"		b = _b; \n"
			"	} \n"
			"	int opCmp(cOpCmp &in other) \n"
			"	{ \n"
			"		return a - other.a; \n"
			"	} \n"
			"	int a; \n"
			"	float b; \n"
			"} \n"
			"class cOpEquals \n"
			"{ \n"
			"	cOpEquals() \n"
			"	{ \n"
			"		a = 0; \n"
			"		b = 0.0; \n"
			"	}	 \n"
			"	cOpEquals(int _a, float _b) \n"
			"	{ \n"
			"		a = _a; \n"
			"		b = _b;	 \n"
			"	} \n"
			"	void set(int _a, float _b) \n"
			"	{ \n"
			"		a = _a; \n"
			"		b = _b; \n"
			"	} \n"
			"	bool opEquals(cOpEquals &in other) \n"
			"	{ \n"
			"		return a == other.a; \n"
			"	} \n"
			"	int a; \n"
			"	float b; \n"
			"} \n"
			"bool TestFind() \n"
			"{ \n"
			"	array<int> A = {5, 8, 3, 2, 0, 0, 2, 1}; \n"
			"	if (A.find(10) != -1) \n"
			"		return false; \n"
			"	if (A.find(0) != 4) \n"
			"		return false; \n"
			"	if (A.find(1, 8) != 1) \n"
			"		return false; \n"
			"	if (A.find(2, 8) != -1) \n"
			"		return false; \n"
			"	array<cOpCmp> CMP(5); \n"
			"	CMP[0].set(0, 0.0); \n"
			"	CMP[1].set(1, 0.0); \n"
			"	CMP[2].set(2, 0.0); \n"
			"	CMP[3].set(3, 0.0); \n"
			"	CMP[4].set(4, 0.0);	\n"
			"	if (CMP.find(cOpCmp(5, 0.0)) != -1) \n"
			"		return false; \n"
			"	if (CMP.find(2, cOpCmp(2, 1.0)) != 2) \n"
			"		return false; \n"
			"	if (CMP.find(3, cOpCmp(2, 1.0)) != -1) \n"
			"		return false; \n"
			"	array<cOpEquals> EQ(5); \n"
			"	EQ[0].set(0, 0.0); \n"
			"	EQ[1].set(1, 0.0); \n"
			"	EQ[2].set(2, 0.0); \n"
			"	EQ[3].set(3, 0.0); \n"
			"	EQ[4].set(4, 0.0); \n"
			"	if (EQ.find(cOpEquals(5, 0.0)) != -1) \n"
			"		return false; \n"
			"	if (EQ.find(2, cOpEquals(2, 1.0)) != 2) \n"
			"		return false; \n"
			"	if (EQ.find(3, cOpEquals(2, 1.0)) != -1) \n"
			"		return false; \n"
			"	return true; \n"
			"} \n"
			"int main() \n"
			"{ \n"
			"	assert( TestSort() ); \n"
			"	assert( TestReverse() ); \n"
			"	assert( TestFind() ); \n"
			"	return 789; \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "assert( main() == 789 ); \n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test array, with objects that don't have default constructor/factory
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"class CTest \n"
			"{ \n"
			"  CTest(int v) {} \n" // With an explicit non-default constructor the compiler won't create the default constructor
			"} \n"
			"array<CTest> arr; \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		bout.buffer = "";
		r = mod->Build();
		if( r > 0 ) 
			TEST_FAILED;
		if( bout.buffer != "script (5, 7) : Error   : Can't instanciate template 'array' with subtype 'CTest'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}	

	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		const char *script = 
			"class T { }; \n"
			"array<T> arr; \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		r = ExecuteString(engine, "array<T> arr(1); \n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test problem with arrays of enums reported by Philip Bennefall
	{
		const char *script = 
			"enum fruit \n"
			"{ \n"
			"  APPLE, ORANGE, BANANA \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  fruit[] basket; \n"
			"  basket.insertLast(APPLE); \n"
			"  basket.insertLast(ORANGE); \n"
			"  basket.sortDesc(); \n"
			"  int index = basket.find(APPLE); \n"
			"  assert( index == 1 ); \n"
			"} \n";

		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test problem with arrays and opEquals reported by Philip Bennefall
	{
		const char *script = 
			"class fish \n"
			"{ \n"
			"  bool opEquals(fish@ other) \n" // handles should be supported too
			"  { \n"
			"    return false; \n"
			"  } \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  fish[] ocean(100); \n"
			"  fish nemo; \n"
			"  int index = ocean.find(nemo); \n"
			"} \n";

		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test problem with arrays and opEquals reported by Philip Bennefall
	{
		const char *script = 
			"class fish \n"
			"{ \n"
			"  bool opEquals(fish@ other) \n"
			"  { \n"
			"    return false; \n"
			"  } \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  fish@[] ocean(100); \n"
			"  for(uint i=0; i<ocean.length(); i++) \n"
			"  { \n"
			"    fish fred; \n"
			"    @(ocean[i]) = fred; \n"
			"  } \n"
			"  fish nemo; \n"
			"  int index = ocean.find(nemo); \n"
			"} \n";

		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		bout.buffer = "";
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test problem with arrays and opAssign reported by Philip Bennefall
	{
		const char *script = 
			"array<uint> a, b = {0,1,2,3}; \n"
			"a.reserve(10); \n"
			"a = b; \n"
			"assert( a.length() == b.length() ); \n"
			"assert( a.length() == 4 ); \n"
			"for( uint n = 0; n < a.length(); n++ ) \n"
			"  assert( a[n] == n ); \n";

		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, script);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Success
	return fail;
}

bool Test2()
{
	bool fail = false;
	COutStream out;

	const char *script =
	"class A                               \n"
	"{                                     \n"
	"	int x;                             \n"
	"}                                     \n"
	"int sum(const array<A>& a)            \n"
	"{                                     \n"
	"	int s = 0;                         \n"
	"	for (uint i=0; i<a.length(); i++)  \n"
	"		s+=a[i].x;                     \n"
	"	return s;                          \n"
	"}                                     \n";

	const char *exec =
	"array<A> As;  \n"
    "As.resize(2); \n"
	"As[0].x = 1;  \n"
	"As[1].x = 2;  \n"
	"sum(As);      \n";

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, false);
	asIScriptModule *module = engine->GetModule("module", asGM_ALWAYS_CREATE);

	module->AddScriptSection("script", script);

	int r = module->Build();
	if( r < 0 )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, exec, module);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

} // namespace

