#include "utils.h"

namespace TestCastOp
{

const char *script = "\
interface intf1            \n\
{                          \n\
  void Test1();            \n\
}                          \n\
interface intf2            \n\
{                          \n\
  void Test2();            \n\
}                          \n\
interface intf3            \n\
{                          \n\
  void Test3();            \n\
}                          \n\
class clss : intf1, intf2  \n\
{                          \n\
  void Test1() {}          \n\
  void Test2() {}          \n\
}                          \n";


// In this test must be possible to call Func both
// with an uint and a double. The path via TestObj2
// must not be considered by the compiler, as that 
// would make: Func(TestObj(TestObj2(2)));
/*
const char *script2 = "\
class TestObj                                     \n\
{                                                 \n\
    TestObj(int a) {this.a = a;}                  \n\
	TestObj(TestObj2 a) {this.a = a.a;}           \n\
	int a;                                        \n\
}                                                 \n\
// This object must not be used to get to TestObj \n\
class TestObj2                                    \n\
{                                                 \n\
    TestObj2(int a) {assert(false);}              \n\
	int a;                                        \n\
}                                                 \n\
void Func(TestObj obj)                            \n\
{                                                 \n\
    assert(obj.a == 2);                           \n\
}                                                 \n\
void Test()                                       \n\
{                                                 \n\
	Func(2);                                      \n\
	Func(2.1);                                    \n\
}                                                 \n";
*/

// In this test it must not be possible to implicitly convert using 
// a path that requires multiple object constructions, e.g.
// Func(TestObj1(TestObj2(2)));
const char *script3 = 
"class TestObj1                 \n"
"{                              \n"
"  TestObj1(TestObj2 a) {}      \n"
"}                              \n"
"class TestObj2                 \n"
"{                              \n"
"  TestObj2(int a) {}           \n"
"}                              \n"
"void Func(TestObj1 obj) {}     \n"
"void Test()                    \n"
"{                              \n"
"  Func(2);                     \n"
"}                              \n";

void TypeToString(asIScriptGeneric *gen)
{
//	int *i = (int*)gen->GetArgPointer(0);
	*(CScriptString**)gen->GetAddressOfReturnLocation() = new CScriptString("type");
}

class A
{
public:
	operator const char * ( ) const { return 0; }
};

class EventSource
{
public:
	EventSource() {refCount = 1; value = 42;};
	virtual ~EventSource() {}
	virtual void AddRef() {refCount++;}
	virtual void Release() {if( --refCount == 0 ) delete this;}
	int refCount;

	int value;
};

class ASConsole : public EventSource
{
public:
	static ASConsole *factory() { return new ASConsole(); }
//	EventSource *opCast() { return this; }
};

ASConsole* c= 0;
bool g_fail = false;
template<class A, class B> B* ASRefCast(A* a)
{
	if( a != c )
		g_fail = true;
		
	// If the handle already is a null handle, then just return the null handle
	if (a==NULL) return NULL;
	// Now try to dynamically cast the pointer to the wanted type
	B* b = dynamic_cast<B*>(a);
	if (b!=NULL) {
			// Since the cast was made, we need to increase the ref counter for the returned handle
			b->AddRef();
	}
//	printf("ASRefCast: returning %p\n", b);
	return b;
}

void addListener(EventSource* source, int mask) 
{
	if( source != c )
		g_fail = true;

//    printf("addListener: source = %p\n", source);
//    printf("addListener: source.value = %d\n", (int)source->value);
//	printf("\n");
}

bool Test()
{
	bool fail = false;
	int r;
	asIScriptEngine *engine;

	CBufferedOutStream bout;
	COutStream out;

	// http://www.gamedev.net/topic/636163-segfault-when-casting-directly/
	{
		c = new ASConsole();

#if defined(_MSC_VER) && _MSC_VER <= 1200
		// MSVC6 was complaining about not finding this symbol because 
		// it didn't see the function being called anywhere
		ASRefCast<ASConsole, EventSource>(c);
		c->Release();
#endif

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterStdString(engine);

		engine->RegisterObjectType("EventSource", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("EventSource", asBEHAVE_ADDREF, "void f()", asMETHOD(EventSource, AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("EventSource", asBEHAVE_RELEASE, "void f()", asMETHOD(EventSource, Release), asCALL_THISCALL);
		engine->RegisterObjectProperty("EventSource", "int value", asOFFSET(EventSource, value));

		engine->RegisterObjectType("ASConsole", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("ASConsole", asBEHAVE_FACTORY, "ASConsole @f()", asFUNCTION(ASConsole::factory), asCALL_CDECL);
		engine->RegisterObjectBehaviour("ASConsole", asBEHAVE_ADDREF, "void f()", asMETHOD(ASConsole, AddRef), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("ASConsole", asBEHAVE_RELEASE, "void f()", asMETHOD(ASConsole, Release), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("ASConsole", asBEHAVE_IMPLICIT_REF_CAST, "EventSource@ f()", asFUNCTION((ASRefCast<ASConsole, EventSource>)), asCALL_CDECL_OBJLAST);

		engine->RegisterGlobalFunction("void addListener(EventSource &inout, const int &in)", asFUNCTION(addListener), asCALL_CDECL);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"enum E { ET_READLINE = 24 } \n"
			"ASConsole @console; \n"
			"void main() \n"
			"{ \n"
			"  addListener(cast<EventSource>(console), ET_READLINE); \n"
			"  EventSource @s = cast<EventSource>(console); \n"
			"  addListener(s, ET_READLINE); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		void** consoleVarAddr = (void**)mod->GetAddressOfGlobalVar(mod->GetGlobalVarIndexByName("console"));
        *consoleVarAddr = c;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( g_fail )
			TEST_FAILED;

		engine->Release();
	}

  	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	int res = 0;
	engine->RegisterGlobalProperty("int res", &res);

	ExecuteString(engine, "res = int(2342.4)");
	if( res != 2342 ) 
		TEST_FAILED;

	ExecuteString(engine, "double tmp = 3452.4; res = int(tmp)");
	if( res != 3452 ) 
		TEST_FAILED;

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	mod->Build();

	r = ExecuteString(engine, "clss c; cast<intf1>(c); cast<intf2>(c);", mod);
	if( r < 0 )
		TEST_FAILED;

	r = ExecuteString(engine, "intf1 @a = clss(); cast<clss>(a).Test2(); cast<intf2>(a).Test2();", mod);
	if( r < 0 )
		TEST_FAILED;

	// Test use of handle after invalid cast (should throw a script exception)
	r = ExecuteString(engine, "intf1 @a = clss(); cast<intf3>(a).Test3();", mod);
	if( r != asEXECUTION_EXCEPTION )
		TEST_FAILED;

	// Don't permit cast operator to remove constness
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "const intf1 @a = clss(); cast<intf2>(a).Test2();", mod);
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "ExecuteString (1, 26) : Error   : No conversion from 'const intf2@' to 'intf2@' available.\n"
					   "ExecuteString (1, 40) : Error   : Illegal operation on 'const int'\n" )
	{
		TEST_FAILED;
		printf("%s", bout.buffer.c_str());
	}

	// It should be allowed to cast null to an interface
	bout.buffer = "";
	r = ExecuteString(engine, "intf1 @a = cast<intf1>(null);", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		TEST_FAILED;
		printf("%s", bout.buffer.c_str());
	}

	//--------------
	// Using constructor as implicit cast operator
	// TODO: Script classes should perhaps allow implicit casts to be implemented as well
/*	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->AddScriptSection(0, "Test2", script2, strlen(script2));
	r = mod->Build(0);
	if( r < 0 )
		TEST_FAILED;
	r = ExecuteString(engine, "Test()");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;
*/
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("Test3", script3, strlen(script3));
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;

	//-------------
	// "test" + string(type) + "\n"
	// "test" + type + "\n" 
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	r = engine->RegisterObjectType("type", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	RegisterScriptString(engine);
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_FACTORY, "string@ f(const type &in)", asFUNCTION(TypeToString), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "type t; string a = \"a\" + string(t) + \"b\";"); 
	if( r < 0 )
		TEST_FAILED;
		
	// Use of constructor is not permitted to implicitly cast to a reference type 
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	r = ExecuteString(engine, "type t; string a = \"a\" + t + \"b\";"); 
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 24) : Error   : No matching operator that takes the types 'string@&' and 'type' found\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Try using the asMETHOD macro with a cast operator
	// The first option fail to compile on MSVC2005 (Thanks Jeff Slutter)
//	engine->RegisterObjectMethod("obj", "void f()", asMETHOD(A, operator const char *), asCALL_THISCALL);
	engine->RegisterObjectMethod("obj", "void f()", asMETHODPR(A, operator const char *, () const, const char *), asCALL_THISCALL);

	engine->Release();

	//-------------------
	// Illegal cast statement
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";

	r = ExecuteString(engine, "uint8 a=0x80; int j=int();");
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 24) : Error   : A cast operator has one argument\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	// Success
 	return fail;
}

} // namespace

