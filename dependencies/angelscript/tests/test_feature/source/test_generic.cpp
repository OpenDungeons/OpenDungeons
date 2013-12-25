
#include <stdarg.h>
#include "utils.h"
#include <sstream>

using std::string;

namespace TestGeneric
{


int obj;

void GenFunc1(asIScriptGeneric *gen)
{
	assert(gen->GetObject() == 0);

//	printf("GenFunc1\n");

	int arg1 = (int)gen->GetArgDWord(0);
	double arg2 = gen->GetArgDouble(1);
	string arg3 = *(string*)gen->GetArgObject(2);

	assert(arg1 == 23);
	assert(arg2 == 23);
	assert(arg3 == "test");

	gen->SetReturnDouble(23);
}

void GenMethod1(asIScriptGeneric *gen)
{
	assert(gen->GetObject() == &obj);

//	printf("GenMethod1\n");

	int arg1 = (int)gen->GetArgDWord(0);
	double arg2 = gen->GetArgDouble(1);

	assert(arg1 == 23);
	assert(arg2 == 23);

	string s("Hello");
	gen->SetReturnObject(&s);
}

void GenAssign(asIScriptGeneric *gen)
{
//	assert(gen->GetObject() == &obj);

	int *obj2 = (int*)gen->GetArgObject(0);
	UNUSED_VAR(obj2);

//	assert(obj2 == &obj);

	gen->SetReturnObject(&obj);
}

void TestDouble(asIScriptGeneric *gen)
{
	double d = gen->GetArgDouble(0);

	assert(d == 23);
}

void TestString(asIScriptGeneric *gen)
{
	string s = *(string*)gen->GetArgObject(0);

	assert(s == "Hello");
}

void GenericString_Construct(asIScriptGeneric *gen)
{
	string *s = (string*)gen->GetObject();

	new(s) string;
}

void GenericString_Destruct(asIScriptGeneric *gen)
{
	string *s = (string*)gen->GetObject();

	s->~string();
}

void GenericString_Assignment(asIScriptGeneric *gen)
{
	string *other = (string*)gen->GetArgObject(0);
	string *self = (string*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

void GenericString_Factory(asIScriptGeneric *gen)
{
	asUINT length = gen->GetArgDWord(0);
	UNUSED_VAR(length);
	const char *s = (const char *)gen->GetArgAddress(1);

	string str(s);

	gen->SetReturnObject(&str);
}

void nullPtr(asIScriptGeneric *gen)
{
	asIScriptObject **intf = (asIScriptObject**)gen->GetAddressOfArg(0);
	assert( *intf == 0 );

	assert(gen->GetArgCount() == 1);

	*(asIScriptObject **)gen->GetAddressOfReturnLocation() = *intf;

	assert(gen->GetReturnTypeId() == gen->GetEngine()->GetTypeIdByDecl("intf@"));
}

bool Test2();

bool Test()
{
	bool fail = Test2();

	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	r = engine->RegisterObjectType("string", sizeof(string), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(GenericString_Construct), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(GenericString_Destruct), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("string", "string &opAssign(string &in)", asFUNCTION(GenericString_Assignment), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterStringFactory("string", asFUNCTION(GenericString_Factory), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("void test(double)", asFUNCTION(TestDouble), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void test(string)", asFUNCTION(TestString), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalFunction("double func1(int, double, string)", asFUNCTION(GenFunc1), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterObjectType("obj", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); assert( r >= 0 );
	r = engine->RegisterObjectMethod("obj", "string mthd1(int, double)", asFUNCTION(GenMethod1), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("obj", "obj &opAssign(obj &in)", asFUNCTION(GenAssign), asCALL_GENERIC); assert( r >= 0 );

	r = engine->RegisterGlobalProperty("obj o", &obj);

	r = engine->RegisterInterface("intf");
	r = engine->RegisterGlobalFunction("intf @nullPtr(intf @)", asFUNCTION(nullPtr), asCALL_GENERIC); assert( r >= 0 );

	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	ExecuteString(engine, "test(func1(23, 23, \"test\"))");

	ExecuteString(engine, "test(o.mthd1(23, 23))");

	ExecuteString(engine, "o = o");

	ExecuteString(engine, "nullPtr(null)");

	engine->Release();

	// Success
	return fail;
}

//--------------------------------------------------------
// This part is going to test the auto-generated wrappers
//--------------------------------------------------------

// This doesn't work on MSVC6. The template implementation isn't good enough.
// It also doesn't work on MSVC2005, it gets confused on const methods that return void. Reported by Jeff Slutter.
// TODO: Need to fix implementation for MSVC2005.
#if !defined(_MSC_VER) || (_MSC_VER > 1200 && _MSC_VER != 1400) 

}
namespace TestGeneric
{

void TestNoArg() {}

void TestStringByVal(std::string val) {
	assert(val == "test");
}

void TestStringByRef(std::string &ref) {
	assert(ref == "test");
}

void TestIntByVal(int val) {
	assert(val == 42);
}

void TestIntByRef(int &ref) {
	assert(ref == 42);
}

int TestRetIntByVal() {
	return 42;
}

int &TestRetIntByRef() {
	static int val = 42;
	return val;
}

std::string TestRetStringByVal() {
	return "test";
}

std::string &TestRetStringByRef() {
	static std::string val = "test";
	return val;
}

void TestOverload(int) {}

void TestOverload(float) {}

class A
{
public:
	A() {id = 0;}
	virtual void a() const {assert(id == 2);}
	int id;
};

class B
{
public:
	B() {}
	virtual void b() {}
};

class C : public A, B
{
public:
	C() {id = 2;}
	~C() {}
	virtual void c(int) {assert(id == 2);}
	virtual void c(float) const {assert(id == 2);}
};

// http://www.gamedev.net/topic/639902-premature-destruction-of-object-in-android/
std::stringstream buf;

class RefCountable
{
public:
	RefCountable() { refCount = 1; buf << "init\n"; }
	virtual ~RefCountable() { buf << "destroy\n"; }
	void addReference() { refCount++; buf << "add (" << refCount << ")\n"; }
	void removeReference() { buf << "rem (" << refCount-1 << ")\n"; if( --refCount == 0 ) delete this; }
	int refCount;
};

class Animable
{
public:
	Animable() {}
};

class Trackable
{
public:
	Trackable() {}
};

class UIControl : public Animable, public Trackable, public RefCountable
{
public:
	UIControl() : Animable(), Trackable(), RefCountable() { test = "hello"; }

	string test;
};

class UIButton : public UIControl
{
public:
	UIButton() : UIControl() {};
};

template<typename T>
T* genericFactory()
{
	return new T();
}

bool Test2()
{
	bool fail = false;
	COutStream out;

	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	RegisterStdString(engine);

	r = engine->RegisterGlobalFunction("void TestNoArg()", WRAP_FN(TestNoArg), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "TestNoArg()");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("void TestStringByVal(string val)", WRAP_FN(TestStringByVal), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "TestStringByVal('test')");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("void TestStringByRef(const string &in ref)", WRAP_FN(TestStringByRef), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "TestStringByRef('test')");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("void TestIntByVal(int val)", WRAP_FN(TestIntByVal), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "TestIntByVal(42)");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("void TestIntByRef(int &in ref)", WRAP_FN(TestIntByRef), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "TestIntByRef(42)");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("int TestRetIntByVal()", WRAP_FN(TestRetIntByVal), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "assert(TestRetIntByVal() == 42)");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("int &TestRetIntByRef()", WRAP_FN(TestRetIntByRef), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "assert(TestRetIntByRef() == 42)");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("string TestRetStringByVal()", WRAP_FN(TestRetStringByVal), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "assert(TestRetStringByVal() == 'test')");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterGlobalFunction("string &TestRetStringByRef()", WRAP_FN(TestRetStringByRef), asCALL_GENERIC); assert( r >= 0 );
	r = ExecuteString(engine, "assert(TestRetStringByRef() == 'test')");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = engine->RegisterObjectType("C", sizeof(C), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("C", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON(C, ()), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("C", asBEHAVE_DESTRUCT, "void f()", WRAP_DES(C), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("C", "void a() const", WRAP_MFN(A, a), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("C", "void b()", WRAP_MFN(B, b), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("C", "void c(int)", WRAP_MFN_PR(C, c, (int), void), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("C", "void c(float) const", WRAP_MFN_PR(C, c, (float) const, void), asCALL_GENERIC); assert( r >= 0 );

	r = ExecuteString(engine, "C c; c.a(); c.b(); c.c(1); c.c(1.1f);");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	engine->Release();

	// http://www.gamedev.net/topic/639902-premature-destruction-of-object-in-android/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		r = engine->RegisterObjectType("UIButton", 0, asOBJ_REF);
		r = engine->RegisterObjectBehaviour("UIButton", asBEHAVE_FACTORY, "UIButton @f()", WRAP_FN(genericFactory<UIButton>), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("UIButton", asBEHAVE_ADDREF, "void f()", WRAP_MFN(UIButton, addReference), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("UIButton", asBEHAVE_RELEASE, "void f()", WRAP_MFN(UIButton, removeReference), asCALL_GENERIC); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "UIButton button;");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		UIButton *but = reinterpret_cast<UIButton*>(mod->GetAddressOfGlobalVar(0));
		if( but == 0 )
			TEST_FAILED;

		if( but->test != "hello" )
			TEST_FAILED;

		buf << "reset\n";
		mod->ResetGlobalVars();

		but = reinterpret_cast<UIButton*>(mod->GetAddressOfGlobalVar(0));
		if( but == 0 )
			TEST_FAILED;

		if( but->test != "hello" )
			TEST_FAILED;

		if( buf.str() != "init\n"
						 "add (2)\n"
						 "rem (1)\n"
						 "reset\n"
						 "rem (0)\n"
						 "destroy\n"
						 "init\n"
						 "add (2)\n"
						 "rem (1)\n" )
		{
			printf("%s", buf.str().c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	return fail;
}
#else
bool Test2()
{
	printf("The test of the autowrapper was skipped due to lack of proper template support\n");
	return false;
}
#endif

} // namespace

