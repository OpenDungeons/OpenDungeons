#include "utils.h"

namespace TestObjHandle
{

static const char * const TESTNAME = "TestObjHandle";



static const char *script1 =
"refclass@ g;                           \n"
"refclass@ c = @g;                      \n"
"class t                                \n"
"{                                      \n"
"  refclass @m;                         \n"
"  void test() {Assert(@m != null);}    \n"
"}                                      \n"
"void TestObjHandle()                   \n"
"{                                      \n"
"   refclass@ b = @refclass();          \n"
// Should generate an exception
// as g isn't initialized yet.
//"   g = b;                              \n"
//"   b = g;                              \n"
// Do a handle assignment
"   @g = @b;                            \n"
// Now an assignment to g is possible
"   g = b;                              \n"
// Compare with null
"   if( @g != null ) {}                  \n"
"   if( null != @g ) {}                   \n"
// Compare with another object
"   if( @g == @b ) {}                       \n"
"   if( @b == @g ) {}                     \n"
// Value comparison
//"   if( g == b );                       \n"
//"   if( b == g );                       \n"
// Assign null to release the object
"   @g = null;                          \n"
"   @g = @b;                            \n"
// Operators
"   b = g + b;                          \n"
// parameter references
"   @g = null;                          \n"
"   TestObjHandleRef(b, @g);            \n"
"   Assert(@g == @b);                   \n"
// return handles
"   @g = null;                          \n"
"   @g = @TestObjReturnHandle(b);       \n"
"   Assert(@g == @b);                   \n"
"   Assert(@TestReturnNull() == null);  \n"
"   Assert(@TestObjReturnHandle(b) != null); \n"
// Test for class members
"   t cl;                               \n"
"   @cl.m = @TestObjReturnHandle(b);    \n"
"   Assert(@cl.m != null);              \n"
"   cl.test();                          \n"
"}                                      \n"
"void TestObjHandleRef(refclass@ i, refclass@ &out o)  \n"
"{                                                     \n"
"   @o = @i;                                           \n"
"}                                                     \n"
"refclass@ TestObjReturnHandle(refclass@ i)            \n"
"{                                                     \n"
"   return i;                                          \n"
"}                                                     \n"
"refclass@ TestReturnNull()                            \n"
"{                                                     \n"
"   return null;                                       \n"
"}                                                     \n";

// Make sure the handle can be explicitly taken for class properties, array members, and global variables
static const char *script5 =
"class C {int val; C() {val = 0;}}      \n"
"class D {C c;}                         \n"
"C g;                                   \n"
"void Test()                            \n"
"{                                      \n"
"   Func(@g);                           \n"
"   Assert(g.val == 1);                 \n"
"   D d;                                \n"
"   Func(@d.c);                         \n"
"   C[] a1(1);                          \n"
"   Func(@a1[0]);                       \n"
"   Assert(a1[0].val == 1);             \n"
"   C@[] a2(1);                         \n"
"   @a2[0] = @C();                      \n"
"   Func(@a2[0]);                       \n"
"   Assert(a2[0].val == 1);             \n"
"}                                      \n"
"void Func(C@ c) {c.val = 1;}           \n";

class CRefClass
{
public:
	CRefClass()
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Construct(%X)\n",this);
		refCount = 1;
	}
	~CRefClass()
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Destruct(%X)\n",this);
	}
	CRefClass &operator=(const CRefClass &o)
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Assign(%X, %X)\n", this, &o);
		return *this;
	}
	int AddRef()
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("AddRef(%X)\n",this);
		return ++refCount;
	}
	int Release()
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Release(%X)\n",this);
		int r = --refCount;
		if( refCount == 0 ) delete this;
		return r;
	}
	static CRefClass &Add(CRefClass &self, CRefClass &other)
	{
//		asIScriptContext *ctx = asGetActiveContext();
//		printf("ln:%d ", ctx->GetCurrentLineNumber());
//		printf("Add(%X, %X)\n", &self, &other);
		return self;
	}
	CRefClass &Do()
	{
		return *this;
	}
	int refCount;
};

CRefClass *Factory()
{
	return new CRefClass;
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}
	bool fail = false;
	int r;
	COutStream out;
	asIScriptEngine *engine;
	asIScriptModule *mod;
	asIScriptContext *ctx;

 	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	RegisterScriptString(engine);

	r = engine->RegisterObjectType("refclass", sizeof(CRefClass), asOBJ_REF); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_FACTORY, "refclass@ f()", asFUNCTION(Factory), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefClass, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("refclass", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefClass, Release), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("refclass", "refclass &opAssign(refclass &in)", asMETHOD(CRefClass, operator=), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("refclass", "refclass &Do()", asMETHOD(CRefClass,Do), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("refclass", "refclass &opAdd(refclass &in)", asFUNCTION(CRefClass::Add), asCALL_CDECL_OBJFIRST); assert(r >= 0);

	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("%s: Failed to compile the script\n", TESTNAME);
	}

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "TestObjHandle()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		TEST_FAILED;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( ctx ) ctx->Release();

	// Call TestObjReturnHandle() from the application to verify that references are updated as necessary
	ctx = engine->CreateContext();
	ctx->Prepare(engine->GetModule(0)->GetFunctionByDecl("refclass@ TestObjReturnHandle(refclass@)"));
	CRefClass *refclass = new CRefClass();

	ctx->SetArgObject(0, refclass);

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);

		TEST_FAILED;
		printf("%s: Execution failed\n", TESTNAME);
	}
	if( refclass->refCount != 2 )
	{
		TEST_FAILED;
		printf("%s: Ref count is wrong\n", TESTNAME);
	}

	refclass->Release();
	if( ctx ) ctx->Release();

	// Test returning a reference to the object from an object method
	r = engine->GarbageCollect();
	asUINT gcCurrentSize;
	engine->GetGCStatistics(&gcCurrentSize, 0, 0);
	assert( gcCurrentSize == 0 ); 

	r = ExecuteString(engine, "refclass ref; ref.Do()");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	engine->Release();

	//--------------------
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, true);
	r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	r = ExecuteString(engine, "Test()", mod);
	if( r != asEXECUTION_FINISHED ) TEST_FAILED;
	engine->Release();

	//----------------------
	// It should be allowed to have a global function return a handle to a const object
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	const char *scriptC = "class T {} const T@ func() {return T();}";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", scriptC, strlen(scriptC));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	engine->Release();

	//---------------------
	// These tests are designed to make sure ambiguities with handles is avoided
	CBufferedOutStream bout;
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	r = engine->RegisterObjectType("A", sizeof(CRefClass), asOBJ_REF); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("A", asBEHAVE_FACTORY, "A@ f()", asFUNCTION(Factory), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("A", asBEHAVE_ADDREF, "void f()", asMETHOD(CRefClass, AddRef), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectBehaviour("A", asBEHAVE_RELEASE, "void f()", asMETHOD(CRefClass, Release), asCALL_THISCALL); assert(r >= 0);
	r = engine->RegisterObjectMethod("A", "A &opAssign(const A &in)", asMETHOD(CRefClass, operator=), asCALL_THISCALL); assert(r >= 0);

	bout.buffer = "";
	r = ExecuteString(engine, "A a; a == null;");    // Should give warning
	if( r < 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; null == a;");    // Should give warning
	if( r < 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; @a == null;");   // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; null == @a;");   // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; @a == a;");      // Should give warning
	if( r < 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; a == @a;");      // Should give warning
	if( r < 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; @a == @a;");     // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A @a = null;");       // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; A @b = a;");     // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; A @b = @a;");    // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; A b = @b;");     // Should give error
	if( r >= 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A @a, b; @a = @b;");  // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A @a, b; @a = b;");   // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A @a, b; a = @b;");   // Should give error
	if( r >= 0 || bout.buffer == "" )
	{
		TEST_FAILED;
	}

	bout.buffer = "";
	r = ExecuteString(engine, "A a; null is a;");    // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}
	bout.buffer = "";
	r = ExecuteString(engine, "A a; a !is null;");    // OK
	if( r < 0 || bout.buffer != "" )
	{
		TEST_FAILED;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

