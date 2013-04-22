#include "utils.h"
#include <sstream>
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
#include "../../../add_on/scriptany/scriptany.h"
#include <iostream>

using namespace std;

namespace TestCompiler
{

static const char * const TESTNAME = "TestCompiler";

// Unregistered types and functions
const char *script1 =
"void testFunction ()                          \n"
"{                                             \n"
" Assert@ assertReached = tryToAvoidMeLeak();  \n"
"}                                             \n";

const char *script2 =
"void CompilerAssert()\n"
"{\n"
"   bool x = 0x0000000000000000;\n"
"   bool y = 1;\n"
"   x+y;\n"
"}";

const char *script3 = "void CompilerAssert(uint8[]@ &in b) { b[0] == 1; }";

const char *script4 = "class C : I {};";

const char *script5 =
"void t() {} \n"
"void crash() { bool b = t(); } \n";

const char *script6 = "class t { bool Test(bool, float) {return false;} }";

const char *script7 =
"class Ship                           \n\
{                                     \n\
	Sprite		_sprite;              \n\
									  \n\
	string GetName() {                \n\
		return _sprite.GetName();     \n\
	}								  \n\
}";

const char *script8 =
"float calc(float x, float y) { Print(\"GOT THESE NUMBERS: \" + x + \", \" + y + \"\n\"); return x*y; }";


const char *script9 =
"void noop() {}\n"
"int fuzzy() {\n"
"  return noop();\n"
"}\n";

const char *script10 =
"void func() {}\n"
"void test() { int v; v = func(); }\n";

const char *script11 =
"class c                                       \n"
"{                                             \n"
"  object @obj;                                \n"
"  void func()                                 \n"
"  {type str = obj.GetTypeHandle();}           \n"
"}                                             \n";

const char *script12 =
"void f()       \n"
"{}             \n"
"               \n"
"void assert()  \n"
"{              \n"
"   2<3?f():1;  \n"
"}              \n";

bool Test2();
bool Test3();
bool Test4();
bool Test5();
bool Test6();
bool Test7();
bool Test8();
bool Test9();
bool TestRetRef();

struct A {
    A() { text = "hello"; }
    static void Constructor(A *self) {new(self) A();}
    static void Destructor(A *memory) {memory->~A();}
    std::string getText() {return this->text;}
    std::string text;
    A getA() {return A();}
};

// For test Philip Bennefall
class CSound
{
public:
	CSound() { refCount = 1; }
	void AddRef() { refCount++; }
	void Release() { if( --refCount == 0 ) delete this; }
	double get_pan() const {return 0;}
	void set_pan(double &) {}
	int refCount;

	static CSound *CSound_fact() {return new CSound();}
};

static void StringFactoryGeneric(asIScriptGeneric *gen) {
  asUINT length = gen->GetArgDWord(0);
  const char *s = (const char*)gen->GetArgAddress(1);
  string str(s, length);
  gen->SetReturnObject(&str);
}

static void StringFactoryConstRefGeneric(asIScriptGeneric *gen) {
  asUINT length = gen->GetArgDWord(0);
  const char *s = (const char*)gen->GetArgAddress(1);
  static string str;
  str = string(s, length);
  gen->SetReturnAddress(&str);
}

static void ConstructStringGeneric(asIScriptGeneric * gen) {
  new (gen->GetObject()) string();
}

static void CopyConstructStringGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetArgObject(0));
  new (gen->GetObject()) string(*a);
}

static void DestructStringGeneric(asIScriptGeneric * gen) {
  string * ptr = static_cast<string *>(gen->GetObject());
  ptr->~string();
}

static void AssignStringGeneric(asIScriptGeneric *gen) {
  string * a = static_cast<string *>(gen->GetArgObject(0));
  string * self = static_cast<string *>(gen->GetObject());
  *self = *a;
  gen->SetReturnAddress(self);
}

static void StringAddGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  string * b = static_cast<string *>(gen->GetArgAddress(0));
  string ret_val = *a + *b;
  gen->SetReturnObject(&ret_val);
}

static void StringLengthGeneric(asIScriptGeneric * gen) {
  string * self = static_cast<string *>(gen->GetObject());
  *static_cast<asUINT *>(gen->GetAddressOfReturnLocation()) = (asUINT)self->length();
}

static bool StringEquals(const std::string& lhs, const std::string& rhs)
{
    return lhs == rhs;
}

static void AddString2IntGeneric(asIScriptGeneric * gen) {
  string * a = static_cast<string *>(gen->GetObject());
  int * b = static_cast<int *>(gen->GetAddressOfArg(0));
  std::stringstream sstr;
  sstr << *a << *b;
  std::string ret_val = sstr.str();
  gen->SetReturnObject(&ret_val);
}

static string alert_buf;
static void AlertGeneric(asIScriptGeneric * gen) {
	string *a = static_cast<string*>(gen->GetArgAddress(0));
	string *b = static_cast<string*>(gen->GetArgAddress(1));
	alert_buf += *a;
	alert_buf += *b;
	alert_buf += "\n";
}

static void String_get_opIndexGeneric(asIScriptGeneric *gen) {
  string * a = static_cast<string *>(gen->GetObject());
  asUINT i = gen->GetArgDWord(0);
  string ret_val = a->substr(i, 1);
  gen->SetReturnObject(&ret_val);
}

static void String_set_opIndexGeneric(asIScriptGeneric *gen) {
  string * a = static_cast<string *>(gen->GetObject());
  asUINT i = gen->GetArgDWord(0);
  string *str = static_cast<string*>(gen->GetArgAddress(1));
  (*a)[i] = (*str)[0];
}

string g_printbuf;
void Print(const string &s)
{
	g_printbuf += s;
}

// For the test with chained method calls
class ChainMe
{
public:
	int X;

	ChainMe() :
		X(0)
	{
	}

	ChainMe &Increase(const int &v)
	{
		X += v;
		return *this;
	}

	static void Construct(void *p)
	{
		new(p) ChainMe();
	}
};

bool Test()
{
	bool fail = false;
	int r;

	fail = Test2() || fail;
	fail = Test3() || fail;
	fail = Test4() || fail;
	fail = Test5() || fail;
	fail = Test6() || fail;
	fail = Test7() || fail;
	fail = Test8() || fail;
	fail = Test9() || fail;
	fail = TestRetRef() || fail;

	asIScriptEngine *engine;
	CBufferedOutStream bout;
	COutStream out;
	asIScriptModule *mod;

	// Test asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, true);
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		// It shall not be possible to do a value assign
		bout.buffer = "";
		mod->AddScriptSection("test",
			"class T { T() {} T &opAssign(const T &in) { return this; } T &opAddAssign(const T &in) { return this; } } \n"
			"void main() { \n"
			"  T t; \n"
			"  t = T(); \n" // fail
			"  t += T(); \n" // fail
			"  @t = T(); \n" // fail (because the variable is not a handle)
			"} \n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (2, 1) : Info    : Compiling void main()\n"
						   "test (4, 5) : Error   : Value assignment on reference types is not allowed. Did you mean to do a handle assignment?\n"
						   "test (5, 5) : Error   : Compound assignment on reference types is not allowed\n"
						   "test (6, 3) : Error   : Expression is not an l-value\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// It shall not be possible to declare function that take ref type by value
		bout.buffer = "";
		mod->AddScriptSection("test",
			"class T { T() {} T &opAssign(const T &in) { return this; } } \n"
			"void func1(T t) {} \n" // fail
			"T func2() { return T(); } \n" // fail
			"void func3(T &t) {} \n" // ok
			"T g; \n"
			"T &func4() { return g; } \n" // ok
			"void func5(T @t) {} \n" // ok
			"T @func6() { return null; } \n" // ok
			);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (2, 1) : Error   : Reference types cannot be passed by value in function parameters\n"
                           "test (3, 1) : Error   : Reference types cannot be returned by value from functions\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// The array add-on should only accept handles for the sub type. This should be controlled by the template callback
		RegisterScriptArray(engine, false);
		bout.buffer = "";
		mod->AddScriptSection("Test",
			"class C {} \n"
			"array<C> arr1; \n" // fail
			"array<C@> arr2; \n" // ok
			);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "Test (2, 7) : Error   : Can't instanciate template 'array' with subtype 'C'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// When passing a ref type to a ? parameter, the compiler should assume it was meant to send the handle
		RegisterScriptAny(engine);
		bout.buffer = "";

		CScriptAny *any = new CScriptAny(engine);
		engine->RegisterGlobalProperty("any a", any);

		mod->AddScriptSection("Test", "class C {} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		
		r = ExecuteString(engine, "a.store(C())", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( (any->GetTypeId() & asTYPEID_OBJHANDLE) == 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "a.store(@C())", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( (any->GetTypeId() & asTYPEID_OBJHANDLE) == 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "C c; a.store(c)", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( (any->GetTypeId() & asTYPEID_OBJHANDLE) == 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		any->Release();
		engine->Release();
	}

	// Test opAssign that returns void
	// It was popping a word too many. Reported by Andrew Ackermann
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { \n"
			"  void opAssign(const T &in v) {} \n"
			"} \n"
			"void main() { \n"
			"  T t = T(); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptFunction *func = mod->GetFunctionByName("main");
		asUINT len;
		asDWORD *bc = func->GetByteCode(&len);
		asBYTE expect[] = 
			{	
				asBC_SUSPEND,asBC_CALL,asBC_STOREOBJ,asBC_ChkNullV,asBC_VAR,asBC_CALL,asBC_STOREOBJ,asBC_PshVPtr,asBC_GETOBJREF,asBC_CALLINTF,asBC_FREE,
				asBC_SUSPEND,asBC_FREE,asBC_RET
			};
		for( asUINT n = 0, i = 0; n < len; )
		{
			asBYTE c = asBYTE(bc[n]);
			if( c != expect[i] )
			{
				TEST_FAILED;
				break;
			}
			n += asBCTypeSize[asBCInfo[c].type];
			if( ++i > sizeof(expect) )
				TEST_FAILED;
		}

		engine->Release();
	}

	// Test
	// http://www.gamedev.net/topic/640966-returning-text-crashes-as-with-mingw-471-but-not-with-441/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

#if !defined(_MSC_VER) || _MSC_VER >= 1700   // MSVC 2012
#if !defined(__GNUC__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)  // gnuc 4.7
        if( GetTypeTraits<A>() != asOBJ_APP_CLASS_CDAK )
            TEST_FAILED;
#endif
#endif

		int r = engine->RegisterObjectType("A", sizeof(A), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("A", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(A::Constructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("A", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(A::Destructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectMethod("A", "string getText()", asMETHOD(A,getText), asCALL_THISCALL);assert( r >= 0 );
		r = engine->RegisterObjectMethod("A", "A getA()", asMETHOD(A,getA), asCALL_THISCALL);assert( r >= 0 );

		r = ExecuteString(engine, "A a; \n"
			                      "string text = a.getA().getText(); \n"
								  "assert( text == 'hello' ); \n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test that integer constants are signed by default
	// http://www.gamedev.net/topic/625735-bizarre-errata-with-ternaries-and-integer-literals/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		g_printbuf = "";
		r = ExecuteString(engine, "float a = ((false?1:0)-(true?1:0)); print('' + a); a += 1; assert( a < 0.005 && a > -0.005 );");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test warnings as error
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 2);

		bout.buffer = "";

		r = ExecuteString(engine, "uint a; a = -12;");
		if( r >= 0  )
			TEST_FAILED;

		if( bout.buffer != "ExecuteString (1, 13) : Warning : Implicit conversion changed sign of value\n"
		                   " (0, 0) : Error   : Warnings are treated as errors by the application\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test check for division -2147483648 by -1
	// http://www.gamedev.net/topic/639703-crash-in-divmod-implementations/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		r = ExecuteString(engine, "int int_min = -2147483648;\n"
								  "int neg_one = -1;\n"
								  "int never_computed = (int_min / neg_one);\n");
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;

		r = ExecuteString(engine, "int int_min = -2147483648;\n"
								  "int neg_one = -1;\n"
								  "int the_same_error = (int_min % neg_one);\n");
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;

		r = ExecuteString(engine, "int int_min = -2147483648;\n"
								  "int neg_one = -1;\n"
								  "uint the_same_error = (uint(int_min) / uint(neg_one));\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "int never_computed = (-2147483648 / -1);\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "int never_computed = (-2147483648 % -1);\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "int64 int_min = -9223372036854775808;\n"
								  "int64 neg_one = -1;\n"
								  "int64 never_computed = (int_min / neg_one);\n");
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;

		r = ExecuteString(engine, "int64 int_min = -9223372036854775808;\n"
								  "int64 neg_one = -1;\n"
								  "int64 the_same_error = (int_min % neg_one);\n");
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;

		r = ExecuteString(engine, "int64 never_computed = (-9223372036854775808 / -1);\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 0);
		r = ExecuteString(engine, "int64 never_computed = (-9223372036854775808 % -1);\n");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( bout.buffer != "ExecuteString (1, 17) : Warning : Implicit conversion changed sign of value\n"
						   "ExecuteString (1, 17) : Warning : Implicit conversion changed sign of value\n"
						   "ExecuteString (1, 25) : Warning : Implicit conversion changed sign of value\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test crash in compiler
	// http://www.gamedev.net/topic/639248-compilation-crash-possibly-on-error-output/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		mod = engine->GetModule("mod1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"import void g(bool dummy, int x = -1) from 'mod2'; \n"
			"void f(bool dummy, int x) \n"
			"{ \n"
			"} \n"
			"void run() \n"
			"{ \n"
			"    f(true, 0); \n"
			"    f(true, 0); \n"
			"    g(false); \n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"import void run() from 'mod1'; \n"
			"void g(bool dummy, int x = -1) \n"
			"{ \n"
			"} \n"
			"class T \n"
			"{ \n"
			"    T() \n"
			"    { \n"
			"        run(); \n"
			"    } \n"
			"}; \n"
			"T Dummy; \n");
		r = mod->Build();
		if( r != asINIT_GLOBAL_VARS_FAILED )
			TEST_FAILED;

		if( bout.buffer != "test (12, 3) : Error   : Failed to initialize global variable 'Dummy'\n"
		                   "test (10, 0) : Info    : Exception 'Unbound function called' in 'T::T()'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test compile error
	// http://www.gamedev.net/topic/637772-small-compiller-bug/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->AddScriptSection("test", "int f() {return 0;;}");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		bout.buffer = "";
		r = mod->AddScriptSection("test", "class A\n"
			"{ \n"
			"    int a; ; \n"
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

	// Test compiler error
	// http://www.gamedev.net/topic/638128-bug-with-show-code-line-after-null-pointer-exception-and-for/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->AddScriptSection("test",
			"void F()\n"
			"{\n"
			"    int a1;\n"
			"    int a2;\n"
			"    int a3;\n"
			"    int a4;\n"
			"    int a5;\n"
			"    F()   '  ;\n"
			"}\n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (8, 11) : Error   : Non-terminated string literal\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test compiling an empty script
	// Reported by Damien French
	{
		asResetGlobalMemoryFunctions();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->AddScriptSection("test", "");
		if( r < 0 )
			TEST_FAILED;
		r = mod->AddScriptSection("test2", 0);
		if( r != asINVALID_ARG )
			TEST_FAILED;
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();

		InstallMemoryManager();
	}

	// Problem reported by Paril101
	// http://www.gamedev.net/topic/636336-member-function-chaining/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		engine->RegisterObjectType("ChainMe", sizeof(ChainMe), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_C);
		engine->RegisterObjectBehaviour("ChainMe", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ChainMe::Construct), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectMethod("ChainMe", "ChainMe &Increase(const int &in)", asMETHOD(ChainMe, Increase), asCALL_THISCALL);
		engine->RegisterObjectProperty("ChainMe", "int x", asOFFSET(ChainMe, X));

		engine->RegisterGlobalFunction("void assert( bool )", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void func() { \n"
			"  func2(ChainMe().Increase(5).Increase(15).Increase(25)); \n"
			"} \n"
			"void func2(const ChainMe &in a) { \n"
			"  assert( a.x == 45 ); \n"
			"} \n");


		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Problem reported by zerochen
	// http://www.gamedev.net/topic/634768-after-unreachable-code-wrong-error-msg/
	{
		const char *script =
			"void dum() {} \n"
			"int dummy() \n"
			"{ \n"
			"  return 0; \n"
			"  dum(); \n"
			"  //return 1; \n"  // Compiler shouldn't complain about paths that don't return
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);

		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "test (2, 1) : Info    : Compiling int dummy()\n"
		                   "test (5, 3) : Warning : Unreachable code\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by _Engine_
	// http://www.gamedev.net/topic/632922-huge-problems-with-precompilde-byte-code/
	{
		const char *script =
			"class SBuilding \n"
			"{ \n"
			"	void ReleasePeople() \n"
			"	{ \n"
			"		SPoint cellij; \n"
			"		if( GetRoadOrFreeCellInAround(cellij) ) {} \n"
			"	} \n"
			"	bool GetRoadOrFreeCellInAround(SPoint&out cellij) \n"
			"	{ \n"
			"		return false; \n"
			"	} \n"
			"} \n"
			"shared class SPoint \n"
			"{ \n"
			"	SPoint@ opAssign(const SPoint&in assign) \n"
			"	{ \n"
			"		return this; \n"
			"	}  \n"
			"} \n";


		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Problem reported by FDsagizi
	// http://www.gamedev.net/topic/632813-compiller-bug/
	{
		const char *script =
			"Test @cur_test; \n"
			"class Test { \n"
			"  void Do() { \n"
			"    cur_test.DoFail(); \n"
			"  } \n"
			"} \n"
			"void DoFail() {} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (3, 3) : Info    : Compiling void Test::Do()\n"
		                   "test (4, 14) : Error   : No matching signatures to 'Test::DoFail()'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by FDsagizi
	// http://www.gamedev.net/topic/632123-compiler-assertion/
	{
		const char *script =
			"void startGame() \n"
			"{ \n"
			"		 array<int> arr; \n"
			"		 string s; \n"
			"		 s +- \n"
			"		 arr.insertLast( 1 ); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (1, 1) : Info    : Compiling void startGame()\n"
		                   "test (5, 7) : Error   : Illegal operation on this datatype\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by Polyak Istvan
	{
		const char *script =
			"class X1 {} \n"
			"class X2 \n"
			"{ \n"
			"    const X1 @ f1 (void) \n"
			"    { \n"
			"        return x1_; \n" // ok
			"    } \n"
			"    const X1 & f2 (void) const \n"
			"    { \n"
			"        return x1_; \n" // ok
			"    } \n"
			"    const X1 & f3 (void) \n"
			"    { \n"
			"        return x1_; \n" // ok
			"    } \n"
			"    const int & f4 (void) \n"
			"    { \n"
			"        return i1_; \n" // ok
			"    } \n"
			"    int & f5 (void) const \n"
			"    { \n"
			"        return i1_; \n" // should fail
			"    } \n"
			"	 X1 & f6 (void) const \n"
			"    { \n"
			"        return x1_; \n" // should fail
			"    } \n"
			"    private X1 x1_; \n"
			"    private int i1_; \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (20, 5) : Info    : Compiling int& X2::f5() const\n"
						   "test (22, 9) : Error   : Can't implicitly convert from 'const int&' to 'int&'.\n"
						   "test (24, 3) : Info    : Compiling X1& X2::f6() const\n"
						   "test (26, 9) : Error   : Can't implicitly convert from 'const X1' to 'X1&'.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by Andrew Ackermann
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

#ifndef AS_MAX_PORTABILITY
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
#else
		engine->RegisterGlobalFunction("void print(const string &in)", WRAP_FN(Print), asCALL_GENERIC);
#endif

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test",
			"class Data {\n"
			"	int x;\n"
			"	Data() {\n"
			"		print('create Data()\\n');\n"
			"	}\n"
			"	~Data() {\n"
			"		print('delete Data()\\n');\n"
			"	}\n"
			"	Data& opAssign(const Data&in other) {\n"
			"		x = other.x;\n"
			"		return this;\n"
			"	}\n"
			"};\n"
			"Data a;\n"
			"Data b;\n"
			"void TestCopyGlobals() {\n"
			"	Data c;\n"
			"	print('--a = b--\\n');\n"
			"	a = b; //Implicitly creates and then deletes a temporary copy\n"
			"	print('--a = c--\\n');\n"
			"	a = c; //Does not create a temporary copy\n"
			"	print('--end--\\n');\n"
			"}\n" );

		g_printbuf = "";

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "TestCopyGlobals()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( g_printbuf != "create Data()\n"
		                  "create Data()\n"
		                  "create Data()\n"
		                  "--a = b--\n"
		                  "--a = c--\n"
		                  "--end--\n"
		                  "delete Data()\n" )
		{
			printf("%s", g_printbuf.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test",
			"int DATE_YEAR { get { return 2012; } } \n"
			"void alert( string t, string v ) { assert( v == '2012' ); } \n"
			"void main() \n"
			"{ \n"
			"  int[] dates(5); \n"
			"  alert('Year', '' + DATE_YEAR);   \n"
			"  dates[3]=DATE_YEAR; \n" // This was storing 3 in the array
			"  alert('Assigned year', '' + dates[3]); \n"
			"} \n" );

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

	// Problem reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);

		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test",
			"string[] get_list() \n"
			"{ \n"
			"  string[]@ null_handle; \n"
			"  return null_handle; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  string[] result=get_list(); \n"
			"} \n");

//		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( string(ctx->GetExceptionString()) != "Null pointer access" )
			TEST_FAILED;
		if( string(ctx->GetExceptionFunction()->GetName()) != "get_list" )
			TEST_FAILED;

		ctx->Release();
		engine->Release();
	}

	// Problem reported by _Vicious_
	// http://www.gamedev.net/topic/625747-multiple-matching-signatures-to/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterScriptString(engine);

		bout.buffer = "";

		engine->RegisterObjectType("Cvar", sizeof(int), asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectBehaviour("Cvar", asBEHAVE_CONSTRUCT, "void f(const string &in, const string &in, const uint)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("Cvar", "void set(string&in)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("Cvar", "void set(float)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("Cvar", "void set(int)", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("Cvar", "void set(double)", asFUNCTION(0), asCALL_GENERIC);

		engine->RegisterObjectType("ElementFormControl", 0, asOBJ_REF | asOBJ_NOCOUNT);
		engine->RegisterObjectMethod("ElementFormControl", "string@ get_value() const", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("ElementFormControl", "void set_value(const string&in)", asFUNCTION(0), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"void func() \n"
			"{ \n"
			"  ElementFormControl @login_form_username; \n"
			"  Cvar mm_user( 'cl_mm_user', '', 0 ); \n"
			"  mm_user.set( login_form_username.value ); \n"
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

	// Problem reported by Ricky C
	// http://www.gamedev.net/topic/625484-c99-hexfloats/#entry4943881
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";

		// Parsing a C99 hex float constant doesn't give error
		// TODO: Maybe one day I'll implement support for this form of float constants
		r = ExecuteString(engine, "float v = 0x219AEFp-24;\n"
								  "v = 0x219AEFp-24;\n"
								  "if( v == 0x219AEFp-24 ) {}\n");
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "ExecuteString (1, 19) : Error   : Expected ',' or ';'\n"
						   "ExecuteString (2, 13) : Error   : Expected ';'\n"
						   "ExecuteString (3, 18) : Error   : Expected ')'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}


	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		bout.buffer = "";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script1, strlen(script1), 0);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void testFunction()\n"
						   "TestCompiler (3, 2) : Error   : Identifier 'Assert' is not a data type\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by ekimr
	{
		const char *script =
			"class END_MenuItem : Widget \n"
			"{ \n"
			"	END_MenuItem() \n"
			"	{ \n"
			"		//super(null); \n"
			"	} \n"
			"}; \n"
			"class Widget \n"
			"{ \n"
			"	Widget( Widget@ parent = null ) \n"
			"	{ \n"
			"	} \n"
			"}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterScriptArray(engine, true);
		RegisterScriptString(engine);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "TestCompiler (3, 2) : Info    : Compiling END_MenuItem::END_MenuItem()\n"
						   "TestCompiler (4, 2) : Error   : Base class doesn't have default constructor. Make explicit call to base constructor\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		// Special string class
		r = engine->RegisterObjectType("string", sizeof(std::string), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
		r = engine->RegisterStringFactory("string", asFUNCTION(StringFactoryGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructStringGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f(const string &in)",    asFUNCTION(CopyConstructStringGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructStringGeneric),  asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(AssignStringGeneric),    asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "uint length() const", asFUNCTION(StringLengthGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "string get_opIndex(uint) const", asFUNCTION(String_get_opIndexGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "void set_opIndex(uint, const string &in)", asFUNCTION(String_set_opIndexGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "string opAdd(int) const", asFUNCTION(AddString2IntGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTION(StringAddGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterGlobalFunction("void alert(string &in, string &in)", asFUNCTION(AlertGeneric), asCALL_GENERIC); assert( r >= 0 );

		// This script should not compile, because true cannot be passed to int& in
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME,
			"void string_contains_bulk(string the_string, string the_bulk)\n"
			"{\n"
			"  string_contains(the_bulk[0], true);\n"
			"}\n"
			"void string_contains(string& in, int& in) {} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void string_contains_bulk(string, string)\n"
						   "TestCompiler (3, 3) : Error   : No matching signatures to 'string_contains(const string&, const bool)'\n"
						   "TestCompiler (3, 3) : Info    : Candidates are:\n"
						   "TestCompiler (3, 3) : Info    : void string_contains(string&in, int&in)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// This script should correctly return the strings
		mod->AddScriptSection(TESTNAME,
			"void main() \n"
			"{ \n"
			"	string test='food'; \n"
			"	test[0]='g'; \n"
			"	for(uint i=0;i<test.length();i++) \n"
			"	  alert('Character ' + (i+1), test[i]); \n"
			"} \n");

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

		if( alert_buf != "Character 1g\n"
			             "Character 2o\n"
						 "Character 3o\n"
						 "Character 4d\n" )
		{
			printf("%s", alert_buf.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Problem reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		// Special string class
		r = engine->RegisterObjectType("string", sizeof(std::string), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
		r = engine->RegisterStringFactory("const string &", asFUNCTION(StringFactoryConstRefGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(ConstructStringGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT,  "void f(const string &in)",    asFUNCTION(CopyConstructStringGeneric), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT,   "void f()",                    asFUNCTION(DestructStringGeneric),  asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("string", "string &opAssign(const string &in)", asFUNCTION(AssignStringGeneric),    asCALL_GENERIC); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
		r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTIONPR(StringEquals, (const string &, const string &), bool), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
#else
		r = engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", WRAP_OBJ_FIRST_PR(StringEquals, (const string &, const string &), bool), asCALL_GENERIC); assert( r >= 0 );
#endif
		r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		// Condition was failing due to the string factory returning a const reference
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME,
			"void func(int value)\n"
			"{\n"
			"  string result= (value<5) ? 'less than 5' : (value>8) ? 'Greater than 8' : 'greater than 5';\n"
			"  assert( result == 'greater than 5' ); \n"
			"}\n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "func(5)", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// http://www.gamedev.net/topic/623846-asccompiler-with-out-asasserts-in-debug/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script =
			"class AAA \n"
			"{ \n"
			"  Car @car; \n"
			"  void Update() \n"
			"  { \n"
			"    if( car !is null ) \n"
			"      car.Update(); \n"
			"  } \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (3, 3) : Error   : Identifier 'Car' is not a data type\n"
						   "TestCompiler (4, 3) : Info    : Compiling void AAA::Update()\n"
						   "TestCompiler (6, 13) : Error   : Both operands must be handles when comparing identity\n"
						   "TestCompiler (7, 10) : Error   : Illegal operation on 'int&'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		mod->AddScriptSection(TESTNAME,
			"class A{} \n"
			"class SomeClass \n"
			"{ \n"
			"         A @a; \n"
			"         void Create() \n"
			"         { \n"
			"                  int some_val = 15; + \n"
			"                  @a = A();\n"
			"         }\n"
			"}\n");

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (5, 10) : Info    : Compiling void SomeClass::Create()\n"
						   "TestCompiler (7, 38) : Error   : Illegal operation on this datatype\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// http://www.gamedev.net/topic/623880-crash-after-get-property-of-null-class-in-function/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script =
			"class BugClass \n"
			"{ \n"
			"         int ID; \n"
			"} \n"
			"void CallBug( BugClass @bc ) \n"
			"{ \n"
			"         int id = bc.ID; \n"
			"} \n"
			"void startGame() \n"
			"{ \n"
			"         CallBug( null ); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "startGame()", mod, ctx);
		if( r != asEXECUTION_EXCEPTION || string(ctx->GetExceptionString()) != "Null pointer access" || string(ctx->GetExceptionFunction()->GetName()) != "CallBug" )
			TEST_FAILED;
		ctx->Release();

		engine->Release();
	}

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, "void f(){\n  int a;\n  a(0)=0;}");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void f()\n"
		                   "TestCompiler (3, 3) : Error   : Expression doesn't form a function call. 'a' is a variable of a non-function type\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	{
		// When passing 'null' to an output parameter the compiler shouldn't warn
		const char *script = "class C {} void func(C @&out) {} \n"
			                 "void main() { \n"
							 "  bool f = true; \n"
							 "  if( f ) \n"
							 "    func(null); \n"
							 "  else \n"
							 "    func(C()); \n"
	                         "}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "TestCompiler (2, 1) : Info    : Compiling void main()\n"
		                   "TestCompiler (7, 10) : Warning : Argument cannot be assigned. Output will be discarded.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	{
		// When passing a double constant to a float arg the compiler shouldn't warn if the value doesn't loose precision
		const char *script = "void func(float) {} \n"
			                 "void main() { \n"
							 "  func(0.3); \n"
	                         "}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script);
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


	{
		const char *script = "class Mind {} \n"
							 "class TA_VehicleInfo {} \n"
							 "class TA_Mind : Mind \n"
		                     "{ \n"
							 "  TA_Mind(TA_VehicleInfo@ vi) \n"
		                     "  { \n"
							 "    VehicleInfo = vi; \n" // script writer did a value assign by mistake
							 "  } \n"
							 "  TA_VehicleInfo@ get_VehicleInfo() const { return m_VehicleInfo; } \n"
							 "  void set_VehicleInfo(TA_VehicleInfo@ info) { @m_VehicleInfo = @info; } \n"
							 "  private TA_VehicleInfo@ m_VehicleInfo; \n"
	                         "}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script, strlen(script), 0);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "TA_VehicleInfo vi; TA_Mind m(vi); \n", mod);
		if( r != asEXECUTION_EXCEPTION )
		{
			TEST_FAILED;
		}

		engine->Release();
	}

	// test 2
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	RegisterScriptArray(engine, true);

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script2, strlen(script2), 0);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "TestCompiler (1, 1) : Info    : Compiling void CompilerAssert()\n"
					   "TestCompiler (3, 13) : Error   : Can't implicitly convert from 'uint' to 'bool'.\n"
					   "TestCompiler (4, 13) : Error   : Can't implicitly convert from 'int' to 'bool'.\n"
					   "TestCompiler (5, 5) : Error   : No conversion from 'bool' to math type available.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// test 3
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script3, strlen(script3), 0);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	// test 4
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "TestCompiler (1, 11) : Error   : Identifier 'I' is not a data type\n" )
		TEST_FAILED;

	// test 5
	RegisterScriptString(engine);
	bout.buffer = "";
	r = ExecuteString(engine, "string &ref");
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : 'string' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "TestCompiler (2, 1) : Info    : Compiling void crash()\n"
	                   "TestCompiler (2, 25) : Error   : Can't implicitly convert from 'void' to 'bool'.\n" )
		TEST_FAILED;

	// test 6
	// Verify that script class methods can have the same signature as
	// globally registered functions since they are in different scope
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("bool Test(bool, float)", asFUNCTION(0), asCALL_GENERIC);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
	r = mod->Build();
	if( r < 0 )
	{
		printf("failed on 6\n");
		TEST_FAILED;
	}

	// test 7
	// Verify that declaring a void variable in script causes a compiler error, not an assert failure
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	bout.buffer = "";
	ExecuteString(engine, "void m;");
	if( bout.buffer != "ExecuteString (1, 6) : Error   : Data type can't be 'void'\n" )
	{
		printf("failed on 7\n");
		TEST_FAILED;
	}

	// test 8
	// Don't assert on implicit conversion to object when a compile error has occurred
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script7, strlen(script7));
	r = mod->Build();
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	if( bout.buffer != "script (3, 2) : Error   : Identifier 'Sprite' is not a data type\n"
					   "script (5, 2) : Info    : Compiling string Ship::GetName()\n"
					   "script (6, 17) : Error   : Illegal operation on 'int&'\n" )
	{
		TEST_FAILED;
	}

	// test 9
	// Don't hang on script with non-terminated string
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script8, strlen(script8));
	r = mod->Build();
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	if( bout.buffer != "script (1, 1) : Info    : Compiling float calc(float, float)\n"
	                   "script (1, 77) : Error   : Multiline strings are not allowed in this application\n"
	                   "script (1, 32) : Error   : No matching signatures to 'Print(string@&)'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// test 10
	// Properly handle error with returning a void expression
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script9, strlen(script9));
	r = mod->Build();
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling int fuzzy()\n"
		               "script (3, 3) : Error   : No conversion from 'void' to 'int' available.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// test 11
	// Properly handle error when assigning a void expression to a variable
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script10, strlen(script10));
	r = mod->Build();
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	if( bout.buffer != "script (2, 1) : Info    : Compiling void test()\n"
		               "script (2, 26) : Error   : Can't implicitly convert from 'void' to 'int'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 12
	// Handle errors after use of undefined objects
	bout.buffer = "";
	engine->RegisterObjectType("type", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script11, strlen(script11));
	r = mod->Build();
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	if( bout.buffer != "script (3, 3) : Error   : Identifier 'object' is not a data type\n"
					   "script (4, 3) : Info    : Compiling void c::func()\n"
                       "script (5, 18) : Error   : Illegal operation on 'int&'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 13
	// Don't permit implicit conversion of integer to obj even though obj(int) is a possible constructor
	bout.buffer = "";
	r = ExecuteString(engine, "uint32[] a = 0;");
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 14) : Error   : Can't implicitly convert from 'const int' to 'uint[]&'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 14
	// Calling void function in ternary operator ?:
	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script12, strlen(script12));
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (4, 1) : Info    : Compiling void assert()\n"
                       "script (6, 4) : Error   : Both expressions must have the same type\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 15
	// Declaring a class inside a function
	bout.buffer = "";
	r = ExecuteString(engine, "class XXX { int a; }; XXX b;");
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : Expected expression value\n"
	                   "ExecuteString (1, 23) : Error   : Identifier 'XXX' is not a data type\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 16
	// Compiler should warn if uninitialized variable is used to index an array
	bout.buffer = "";
	const char *script_16 = "void func() { int[] a(1); int b; a[b] = 0; }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script_16, strlen(script_16));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 36) : Warning : 'b' is not initialized.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 17
	// Compiler should warn if uninitialized variable is used with post increment operator
	bout.buffer = "";
	const char *script_17 = "void func() { int a; a++; }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script_17, strlen(script_17));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
		               "script (1, 23) : Warning : 'a' is not initialized.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 18
	// Properly notify the error of comparing boolean operands
	bout.buffer = "";
	r = ExecuteString(engine, "bool b1,b2; if( b1 <= b2 ) {}");
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 20) : Warning : 'b1' is not initialized.\n"
                       "ExecuteString (1, 20) : Warning : 'b2' is not initialized.\n"
                       "ExecuteString (1, 20) : Error   : Illegal operation on this datatype\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 19 - moved to test_scriptretref

	// Test 20
	// Don't crash on invalid script code
	bout.buffer = "";
	const char *script20 =
		"class A { A @b; } \n"
		"void test()       \n"
		"{ A a; if( @a.b == a.GetClient() ) {} } \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script20", script20, strlen(script20));
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "script20 (2, 1) : Info    : Compiling void test()\n"
	                   "script20 (3, 22) : Error   : No matching signatures to 'A::GetClient()'\n"
	                   "script20 (3, 17) : Warning : The operand is implicitly converted to handle in order to compare them\n"
	                   "script20 (3, 17) : Error   : No conversion from 'const int' to 'A@' available.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 21
	// Don't crash on undefined variable
	bout.buffer = "";
	const char *script21 =
		"bool MyCFunction() {return true;} \n"
		"void main() \n"
		"{ \n"
		"	if (true and MyCFunction( SomethingUndefined )) \n"
		"	{ \n"
		"	} \n"
		"} \n";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script21", script21, strlen(script21));
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "script21 (2, 1) : Info    : Compiling void main()\n"
					   "script21 (4, 28) : Error   : 'SomethingUndefined' is not declared\n"
					   "script21 (4, 11) : Error   : No conversion from 'int' to 'bool' available.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 22
	bout.buffer = "";
	const char *script22 =
		"class Some{} \n"
		"void Func(Some@ some) \n"
		"{ \n"
		"if( some is null) return; \n"
		"Func_(null); \n"
		"} \n"
		"void Func_(uint i) \n"
		"{ \n"
		"} \n";

	r = mod->AddScriptSection("22", script22);
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "22 (2, 1) : Info    : Compiling void Func(Some@)\n"
	                   "22 (5, 1) : Error   : No matching signatures to 'Func_(<null handle>)'\n"
					   "22 (5, 1) : Info    : Candidates are:\n"
					   "22 (5, 1) : Info    : void Func_(uint)\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 23 - don't assert on invalid condition expression
	bout.buffer = "";
	const char *script23 = "openHandle.IsValid() ? 1 : 0\n";

	r = ExecuteString(engine, script23);
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 1) : Error   : 'openHandle' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 24 - don't assert on invalid return statement
	bout.buffer = "";
	const char *script24 = "string SomeFunc() { return null; }";
	r = mod->AddScriptSection("24", script24);
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "24 (1, 1) : Info    : Compiling string SomeFunc()\n"
		               "24 (1, 28) : Error   : Can't implicitly convert from '<null handle>' to 'string'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Test 25 - moved to test_scriptretref

	// Test 26 - don't crash on invalid script
	bout.buffer = "";
	const char *script26 = "void main() { main(anyWord)+main(anyWord); }";
	r = mod->AddScriptSection("26", script26);
	r = mod->Build();
	if( r >= 0 ) TEST_FAILED;
	if( bout.buffer != "26 (1, 1) : Info    : Compiling void main()\n"
	                   "26 (1, 20) : Error   : 'anyWord' is not declared\n"
	                   "26 (1, 29) : Error   : No matching signatures to 'main(int)'\n"
					   "26 (1, 29) : Info    : Candidates are:\n"
					   "26 (1, 29) : Info    : void main()\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	// Test 27 - don't crash on missing behaviours
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		// We don't register the assignment behaviour
		r = engine->RegisterObjectType("derp", 0, asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("derp", asBEHAVE_FACTORY,    "derp@ f()",                 asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("derp", asBEHAVE_FACTORY,    "derp@ f(int &in)",          asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("derp", asBEHAVE_FACTORY,    "derp@ f(const derp &in)",   asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("derp", asBEHAVE_ADDREF,     "void f()",                  asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("derp", asBEHAVE_RELEASE,    "void f()",                  asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "derp wtf = 32;");
		r = mod->Build();
		if( r >= 0 || bout.buffer != "test (1, 10) : Info    : Compiling derp wtf\n"
		                             "test (1, 12) : Error   : Can't implicitly convert from 'const int' to 'derp&'.\n"
		                             "test (1, 12) : Error   : There is no copy operator for the type 'derp' available.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test 28 - if with empty statement should give error
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		r = ExecuteString(engine, "if(true); if(true) {} else;");
		if( r >= 0 )
		{
			TEST_FAILED;
		}

		if( bout.buffer != "ExecuteString (1, 9) : Error   : If with empty statement\n"
			               "ExecuteString (1, 27) : Error   : Else with empty statement\n")
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test 29 - function overloads with multiple matches must display matches
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script = "int func() { return 0; }\n"
			                 "float func() { return 0; }\n"
							 "void main() { func(); }\n";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);
		int r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "s (2, 1) : Error   : A function with the same name and parameters already exist\n"
		                   "s (3, 1) : Info    : Compiling void main()\n"
		                   "s (3, 15) : Error   : Multiple matching signatures to 'func()'\n"
		                   "s (3, 15) : Info    : int func()\n"
		                   "s (3, 15) : Info    : float func()\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test 30 - proper handling of incorrect script
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);

		const char *script = "void main() \n"
							 "{ \n"
							 "  string t = string(ti); \n" //ti is undefined
							 "} \n";

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);
		int r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "s (1, 1) : Info    : Compiling void main()\n"
						   "s (3, 21) : Error   : 'ti' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test to make sure compilation error is properly handled
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptString_Generic(engine);

		int r = ExecuteString(engine, "MissingFunction('test')");
		if( r >= 0 )
		{
			TEST_FAILED;
			printf("%s: ExecuteString() succeeded even though it shouldn't\n", TESTNAME);
		}

		engine->Release();
	}

	// Give useful error message when no matching function is found
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"void test(int a) { }        \n"
			"void test(float a) { }      \n"
			"void test(bool c) { }       \n"
			"class Test {                \n"
			"    void test(int a) { }    \n"
			"    void test(float a) { }  \n"
			"    void test(bool c) { }   \n"
			"}                           \n"
			"void main() {               \n"
			"    test();                 \n"
			"    Test test;              \n"
			"    test.test();            \n"
			"}                           \n";

		mod->AddScriptSection(0, script);

		bout.buffer = "";
		int r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != " (9, 1) : Info    : Compiling void main()\n"
						   " (10, 5) : Error   : No matching signatures to 'test()'\n"
						   " (10, 5) : Info    : Candidates are:\n"
						   " (10, 5) : Info    : void test(int)\n"
						   " (10, 5) : Info    : void test(float)\n"
					   	   " (10, 5) : Info    : void test(bool)\n"
						   " (12, 10) : Error   : No matching signatures to 'Test::test()'\n"
						   " (12, 10) : Info    : Candidates are:\n"
						   " (12, 10) : Info    : void Test::test(int)\n"
						   " (12, 10) : Info    : void Test::test(float)\n"
						   " (12, 10) : Info    : void Test::test(bool)\n" )
		{
			TEST_FAILED;
			printf("%s", bout.buffer.c_str());
		}

		engine->Release();
	}

	//
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

		RegisterScriptString(engine);

		engine->RegisterObjectType("sound", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_FACTORY, "sound @f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("sound", "bool get_playing()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("sound", "int get_count()", asFUNCTION(0), asCALL_GENERIC);

		const char *script = "void main() \n"
							 "{ \n"
							 "  sound s; \n"
							 "  for(;s.playing;) {}\n"
							 "  while(s.playing) {} \n"
							 "  do {} while (s.playing); \n"
							 "  if(s.playing) {} \n"
							 "  s.playing ? 0 : 1; \n"
							 "  switch(s.count) {case 0:} \n"
							 "}\n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	//
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		RegisterScriptString(engine);
		const char *scriptMain =
		"void error()"
		"{"
		"\"\" + (a.a() - b);"
		"}";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("scriptMain", scriptMain, strlen(scriptMain));
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "scriptMain (1, 1) : Info    : Compiling void error()\n"
						   "scriptMain (1, 20) : Error   : 'a' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		engine->Release();
	}

	//
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		const char *script =
			"class Hoge \n"
			"{ \n"
			"  int mValue; \n"
			"  Hoge() \n"
			"  { \n"
			"    mValue = 0; \n"
			"  } \n"
			"  Hoge@ opAssign(const Hoge &in aObj) \n"
			"  { \n"
			"    mValue = aObj.mValue; \n"
			"    return @this; \n"
			"  } \n"
			"}; \n"
			"void main() \n"
			"{ \n"
			"  Hoge a = Hoge(); \n"
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

		engine->Release();
	}

	//
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);
		bout.buffer = "";

		const char *script =
			"class Test \n"
			"{ \n"
			"  const string @get_id() \n"
			"  { \n"
			"    return @'test'; \n"
			"  } \n"
			"} \n"
			"void getClauseDesc(const string &in s) \n"
			"{ \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  Test t; \n"
			"  getClauseDesc(t.id); \n"
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
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterObjectType("Entity", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("Entity", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("Entity", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);

		engine->RegisterObjectType("EntityArray", 0, asOBJ_REF);
		engine->RegisterObjectBehaviour("EntityArray", asBEHAVE_FACTORY, "EntityArray @f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("EntityArray", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("EntityArray", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
		engine->RegisterObjectMethod("EntityArray", "Entity@ &opIndex(const uint)", asFUNCTION(0), asCALL_GENERIC);

		engine->RegisterGlobalFunction("Entity @DeleteEntity(Entity &in)", asFUNCTION(0), asCALL_GENERIC);

		// Because the DeleteEntity is taking &in, the object must be copied to a variable
		// to make sure the original object is not modified by the function. Because the
		// Entity doesn't have a default factory, this is supposed to fail
		const char *script =
			"void func() { \n"
			"EntityArray arr; \n"
			"Entity @temp = @arr[0]; \n"
			"DeleteEntity(temp); \n"
			"DeleteEntity(arr[0]); \n"
			"}; \n";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "script (1, 1) : Info    : Compiling void func()\n"
						   "script (4, 14) : Error   : No default constructor for object of type 'Entity'.\n"
						   "script (4, 14) : Error   : There is no copy operator for the type 'Entity' available.\n"
						   "script (5, 14) : Error   : No default constructor for object of type 'Entity'.\n"
						   "script (5, 14) : Error   : There is no copy operator for the type 'Entity' available.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);

		mod->AddScriptSection("test", "class C { int x; int get_x() {return x;} }\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);

		bout.buffer = "";
		mod->AddScriptSection("test", "interface ITest {}\n class Test {ITest t;}\n class Test2 : Test {}\n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "test (2, 20) : Error   : Data type can't be 'ITest'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Avoid assert failure on undeclared variables
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		const char *script =
			"void my_method() \n"
			"{ \n"
			"    int[] arr; \n"
			"    if(arr[unexisting_var-1]==1) \n"
			"    { \n"
			"    } \n"
			"} \n";

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (1, 1) : Info    : Compiling void my_method()\n"
		                   "script (4, 12) : Error   : 'unexisting_var' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	/////////////////
	{
		const char *script =
			"void main()\n"
			"{\n"
			"  while(turn()) {}\n"
			"}\n"

			"void turn()\n"
			"{\n"
			"}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterStdString(engine);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (1, 1) : Info    : Compiling void main()\n"
		                   "script (3, 9) : Error   : Expression must be of boolean type\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	//////////////
	{
		const char *script =
			"class irc_event\n"
			"{\n"
			"	irc_event() \n"
			"	{\n"
			"       // apparently the following code will make AngelScript segfault rather than throw an error\n"
			"		command=params='NULL';\n"
			"	}\n"
			"	void set_command(string@[] i)   {command=i;}\n"
			"	void set_params(string@ i)      {params=i;}\n"
			"	string@[] get_command() {return command;    }\n"
			"	string@ get_params()    {return params;     }\n"
			"	string@[] command;\n"
			"	string params;\n"
			"}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterScriptString(engine);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (3, 2) : Info    : Compiling irc_event::irc_event()\n"
		                   "script (6, 10) : Error   : No matching signatures to 'irc_event::set_command(string)'\n"
		                   "script (6, 10) : Info    : Candidates are:\n"
		                   "script (6, 10) : Info    : void irc_event::set_command(string@[])\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	//////////////
	{
		const char *script =
			"enum wf_type \n"
			"{ \n"
			"  sawtooth=1, \n"
			"  square=2, \n"
			"  sine=3 \n"
			"} \n"
			"class tone_synth \n"
			"{ \n"
			"  void set_waveform_type(wf_type i) {} \n"
			"} \n"
			"void main () \n"
			"{ \n"
			"  tone_synth t; \n"
			"  t.waveform_type = sine; \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void sine()", asFUNCTION(0), asCALL_GENERIC);

		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (11, 1) : Info    : Compiling void main()\n"
		                   "script (14, 19) : Error   : No matching signatures to 'tone_synth::set_waveform_type(sine)'\n"
		                   "script (14, 19) : Info    : Candidates are:\n"
		                   "script (14, 19) : Info    : void tone_synth::set_waveform_type(wf_type)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	//////////////
	{
		const char *script =
			"class Obj {}; \n"
			"class Hoge \n"
			"{ \n"
			"    const Obj obj()const { return Obj(); } \n"
			"} \n"
			"class Foo \n"
			"{ \n"
			"    Foo() \n"
			"    { \n"
			"        Hoge h; \n"
			"        Obj tmpObj = h.obj(); /* OK */ \n"
			"        mObj = h.obj(); /* Build failed */ \n" // this should work
			"    } \n"
			"    Obj mObj; \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void sine()", asFUNCTION(0), asCALL_GENERIC);

		bout.buffer = "";
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

		engine->Release();
	}

	//////////////
	{
		const char *script =
			"class dummy \n"
			"{ \n"
			"int x; \n"
			"dummy(int new_x) \n"
			"{ \n"
			"x=new_x; \n"
			"} \n"
			"} \n"
			"  \n"
			"void main() \n"
			"{ \n"
			"alert('Result', '' + bad.x + ''); \n"
			"dummy bad(15); \n"
			"alert('Result', '' + bad.x + ''); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);


		bout.buffer = "";
		asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (10, 1) : Info    : Compiling void main()\n"
		                   "script (12, 22) : Error   : 'bad' is not declared\n"
		                   "script (13, 7) : Error   : 'bad' is already declared\n"
		                   "script (14, 25) : Error   : 'x' is not a member of 'int'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	//////////////
	{
		const char *script =
			"void main() { \n"
			"  int r = 2; \n"
			"  while(r-- > 0) {}; } \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
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

		engine->Release();
	}

	/////////////////////
	// This test validates that the temporary variable used to store the return
	// value while the output parameter is evaluated isn't overwritten
	{
		const char *script =
			"class obj {} \n"
			"bool getPendingMats(obj@&out TL) \n"
			"{ \n"
			"  return false; \n"
			"} \n"
			"void paintFloor() \n"
			"{ \n"
			"  obj@[] center(4); \n"
			"  bool bb = getPendingMats(center[3]); \n"
			"  assert( bb == false ); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		bout.buffer = "";
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

		r = ExecuteString(engine, "paintFloor()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test the parsing of doubles
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		double d;
		engine->RegisterGlobalProperty("double d", &d);

		ExecuteString(engine, "d = 0.1234567890123456789");

		if( !CompareDouble(d, 0.1234567890123456789) )
		{
			cout << "Got: d = " << d << endl;
			TEST_FAILED;
		}

		ExecuteString(engine, "d = 1.0e300");

		if( !CompareDouble(d/1.0e300, 1.0) )
		{
			cout << "Got: d = " << d << ", d/1e300 = " << (d/1.0e300) << " d/1e300 - 1 = " << (d/1.0e300-1.0) << endl;
			TEST_FAILED;
		}

		engine->Release();
	}

	// Make sure the deferred parameters are processed in the switch condition
	{
		const char *script =
			"int[] level1(9); \n"
			"void move_x() \n"
			"{ \n"
			"    switch(level1[1]) \n"
			"    { \n"
			"    case 2: \n"
			"      break; \n"
			"    } \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		bout.buffer = "";
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

		engine->Release();
	}

	// Test float numbers starting with .
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, "assert( .42 == 0.42 ); assert( .42f == 0.42f )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test return of handle to array
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"class Test \n"
			"{ \n"
			"  array<int> @retArray() { return array<int>(); } \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test complex expression
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"class vec { \n"
			"  float angleAt(float) {return 0;} \n"
			"} \n"
			"float easeIn(float) {return 0;} \n"
			"class t { \n"
			"  float rotation; \n"
			"  vec path; \n"
			"  float alpha; \n"
			"} \n"
			"class a { \n"
			"  t _feuilleRonce00; \n"
			"  void main() \n"
			"  { \n"
			"    t[] plant; \n"
			"    if( true ) \n"
			"      _feuilleRonce00.rotation = _feuilleRonce00.rotation + (((plant[0].path.angleAt(easeIn(plant[0].alpha) - 0.1f)) - 0.5f) - _feuilleRonce00.rotation) * 0.1f; \n"
			"    else \n"
			"      _feuilleRonce00.rotation = _feuilleRonce00.rotation + (((plant[0].path.angleAt(easeIn(plant[0].alpha) - 0.1f))) - _feuilleRonce00.rotation) * 0.1f; \n"
			"}} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test parser error
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"void main() \n"
			"{ \n"
			"  int[] _countKill; \n"
			"  _countKill[12)++; \n"
			"} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "script (1, 1) : Info    : Compiling void main()\n"
		                   "script (4, 16) : Error   : Expected ']'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test - Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterStdString(engine);

		engine->RegisterObjectType("sound", 0, asOBJ_REF);
#ifndef AS_MAX_PORTABILITY
		engine->RegisterObjectBehaviour("sound", asBEHAVE_FACTORY, "sound @f()", asFUNCTIONPR(CSound::CSound_fact, (), CSound *), asCALL_CDECL);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_ADDREF, "void f()", asMETHODPR(CSound, AddRef, (), void), asCALL_THISCALL);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_RELEASE, "void f()", asMETHODPR(CSound, Release, (), void), asCALL_THISCALL);
		engine->RegisterObjectMethod("sound", "double get_pan() const", asMETHODPR(CSound, get_pan, () const, double), asCALL_THISCALL);
		engine->RegisterObjectMethod("sound", "void set_pan(double &in)", asMETHODPR(CSound, set_pan, (double &), void), asCALL_THISCALL);
#else
		engine->RegisterObjectBehaviour("sound", asBEHAVE_FACTORY, "sound @f()", WRAP_FN_PR(CSound::CSound_fact, (), CSound *), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_ADDREF, "void f()", WRAP_MFN_PR(CSound, AddRef, (), void), asCALL_GENERIC);
		engine->RegisterObjectBehaviour("sound", asBEHAVE_RELEASE, "void f()", WRAP_MFN_PR(CSound, Release, (), void), asCALL_GENERIC);
		engine->RegisterObjectMethod("sound", "double get_pan() const", WRAP_MFN_PR(CSound, get_pan, () const, double), asCALL_GENERIC);
		engine->RegisterObjectMethod("sound", "void set_pan(double &in)", WRAP_MFN_PR(CSound, set_pan, (double &), void), asCALL_GENERIC);
#endif

		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"void main() \n"
			"{ \n"
			" sound b; \n"
			" int hits = 0; \n"
			" if(b.pan >= -5) \n"
			" { \n"
			" } \n"
			" switch(hits) \n"
			" { \n"
			" case 10:  \n"
			"  break; \n"
			" case 15:  \n"
			"  break; \n"
			" case 20:\n"
			"  break; \n"
			" } \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test - Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, false);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"bool freq_ms(string &in freqs, double &in length) \n"
			"{ \n"
			"  assert( freqs == '524' ); \n"
			"  assert( length == 1000 ); \n"
			"  return false; \n"
			"} \n"
			"class tone_player \n"
			"{ \n"
			"  double freq; \n"
			"  double ms; \n"
			"  tone_player() \n"
			"  { \n"
			"    freq=524; \n"
			"    ms=1000; \n"
			"  } \n"
			"  void play_tone() \n"
			"  { \n"
			"    freq_ms(''+freq,ms); \n"
			"  } \n"
			"} \n"
			);

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "tone_player tp; tp.play_tone();", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test - Philip Bennefall
	{
		const char *script =
			"class Technique {\n"
			"  string hitsound;\n"
			"}\n"
			"Technique@ getTechnique() {return @Technique();}\n"
			"void main() {\n"
			"  string t = getTechnique().hitsound;\n"
			"}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterStdString(engine);
		RegisterScriptDictionary(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main();", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test - reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
#ifndef AS_MAX_PORTABILITY
		engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(Print), asCALL_CDECL);
#else
		engine->RegisterGlobalFunction("void print(const string &in)", WRAP_FN(Print), asCALL_GENERIC);
#endif

		const char *script =
			"double round(double d, int i) {return double(int64(d*1000+0.5))/1000;}\n"
	//		"string input_box(string &in, string &in) {return '';}\n"
	//		"int string_to_number(string) {return 0;}\n"
			"void main()\n"
			"{\n"
			//"  show_game_window('Golden Ratio');\n"
			"  \n"
			"  double numberA=0, numberB=1;\n"
			"  double sum_of_ratios=0;\n"
			"  int sequence_length=1475; \n" // 1475 is the largest we can go without breaking the limits of what a double can hold
			"  \n"
			"  for(int i=0; i<sequence_length; i++)\n"
			"  { \n"
			"    double temp=numberB;\n"
			"    numberB=numberA+numberB;\n"
			"    numberA=temp;\n"
			"    \n"
	//		"    print('A:'+numberA+', B:'+numberB+'\\n'); \n"
			"    sum_of_ratios+=round(numberB/numberA, 3); \n"
	//		"    print(sum_of_ratios+'\\n'); \n"
			"  } // end for. \n"
			"  \n"
			"  double average_of_ratios=sum_of_ratios/sequence_length; \n"
			"  average_of_ratios=round(average_of_ratios, 3); \n"
//			"  assert(average_of_ratios == 1.618); \n"
			"  print('The average of first '+sequence_length+' ratios of Fibonacci number is: '+average_of_ratios+'.\\n'); \n"
			//"  alert('Average', 'The average of first '+sequence_length+' ratios of Fibonacci number is: '+average_of_ratios+'.'); \n"
			//"  exit(); \n"
			"}\n";

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		int r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		g_printbuf = "";

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( g_printbuf != "The average of first 1475 ratios of Fibonacci number is: 1.618.\n" )
		{
			printf("%s", g_printbuf.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test invalid script
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"class B {}\n"
			"void func() \n"
			"{ \n"
			"  B @b = cast<B>( typo.createInstance() ); //typo is obviously not an object.  \n"
			"} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (2, 1) : Info    : Compiling void func()\n"
		                   "script (4, 19) : Error   : 'typo' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test ambigious names
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"void name(uint8 a) { } \n"
			"void main() \n"
			"{ \n"
			"  uint8 name; \n"
			"  name(7); \n"
			"  ::name(7); \n"
			"} \n");

		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (2, 1) : Info    : Compiling void main()\n"
                           "script (5, 3) : Error   : Expression doesn't form a function call. 'name' is a variable of a non-function type\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test function overloading
	// This was failing because the code didn't filter out the method with only one parameter before starting
	// the match making. As it's first parameter was a better match it then made the code ignore the correct
	// method.
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"class T { \n"
			"  uint mthd(int a, bool b) {return 0;} \n"
			"  bool mthd(uint a) {return false;} \n"
			"} \n"
			"void main() { \n"
			"  T t; \n"
			"  t.mthd(1, true); \n"
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

	// Test value copy during decl
	// http://www.gamedev.net/topic/618412-memory-leak-when-doing-an-assign-operation-with-a-handle/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script",
			"class T { \n"
			"  T() {} \n"
		//	"  T(int v) {} \n"
			"  T &opAssign(const T&in o) {return this;} \n"
			"} \n"
			"T @Get() { return T(); } \n"
			"void main() { \n"
			"  T @t1 = Get(); \n"
			"  T t2 = t1; \n"
			"} \n");

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

	// Test extremely long expressions
	// Previously this was failing due to the recursiveness in the compiler
	// Reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		string script;
		script.reserve(20000);
		script += "int a = 1; \na = ";
		for( asUINT n = 0; n < 9500; n++ )
			script += "a+";
		script += "a; \nassert( a == 9501 ); \n";

		r = ExecuteString(engine, script.c_str());
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}


	// This test caught a problem when the script code allocated a script class,
	// which in turn used nested contexts to initialize some members. The VM
	// hadn't updated the members with the stack pointer/program pointer before
	// the nested call so the memory was overwritten.
	// Reported by Andrew Ackermann
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"shared class Alignment { \n"
			"  Alignment(int lt, float lp, int lx, \n"
			" 		     int tt, float tp, int tx, \n"
			"		     int rt, float rp, int rx, \n"
			"		     int bt, float bp, int bx) \n"
			"  { \n"
			"    assert(lt == AS_Left); \n"
			"    assert(tt == AS_Bottom); \n"
			"    assert(rt == AS_Right); \n"
			"    assert(bt == AS_Bottom); \n"
			"    assert(lp == 3.14f); \n"
			"    assert(tp == 1.43f); \n"
			"    assert(rp == 4.13f); \n"
			"    assert(bp == 4.34f); \n"
			"    assert(lx == 42); \n"
			"    assert(tx == 53); \n"
			"    assert(rx == 64); \n"
			"    assert(bx == 75); \n"
			"  }\n"
			"  AlignedPoint left; \n"
		//	"  AlignedPoint right; \n"
		//	"  AlignedPoint top; \n"
		//	"  AlignedPoint bottom; \n"
		//	"  double aspectRatio; \n"
		//	"  double aspectHorizAlign; \n"
		//	"  double aspectVertAlign; \n"
			"} \n"
			"shared class AlignedPoint { \n"
		//	"  int type; \n"
		//	"  float percent; \n"
		//	"  int pixels; \n"
		//	"  int size; \n"
			"  \n"
			"  AlignedPoint() { \n"
		//	"    type = AS_Left; \n"
		//	"    pixels = 0; \n"
		//	"    percent = 0; \n"
		//	"    size = 0; \n"
			"  } \n"
			"} \n"
			"shared enum AlignmentSide \n"
			"{ \n"
			"  AS_Left, AS_Right, AS_Top = AS_Left, AS_Bottom = AS_Right \n"
			"} \n"
			"class Fault { \n"
			"  Alignment @get_alignment() {return A;} \n"
			"  void set_alignment(Alignment@ value) {@A = value;} \n"
			"  Fault() { \n"
			"    a = 3.14f; \n"
			"    b = 1.43f; \n"
			"    c = 4.13f; \n"
			"    d = 4.34f; \n"
			"    @alignment = Alignment( \n"
			"                    AS_Left,   a + 0.0f, 42, \n"
			"                    AS_Bottom, b + 0.0f, 53, \n"
			"                    AS_Right,  c + 0.0f, 64, \n"
			"                    AS_Bottom, d + 0.0f, 75); \n"
			"  } \n"
			"  Alignment @A; \n"
			"  float a; float b; float c; float d; \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "Fault f()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Success
 	return fail;
}


//------------------------------------
// Test 2 was reported by Dentoid
float add(float &a, float &b)
{
	return a+b;
}

void doStuff(float a, float b)
{
}

bool Test2()
{
	if( strstr(asGetLibraryOptions(), " AS_MAX_PORTABILITY ") )
		return false;

#if defined(__GNUC__) && defined(__amd64__)
	// TODO: Add this support
	// Passing non-complex objects by value is not yet supported, because
	// it means moving each property of the object into different registers
	return false;
#endif

	bool fail = false;
	int r;
	COutStream out;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->RegisterObjectType( "Test", sizeof(float), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_FLOAT );
	engine->RegisterObjectMethod( "Test", "Test opAdd(Test &in) const", asFUNCTION(add), asCALL_CDECL_OBJFIRST);
	engine->RegisterGlobalFunction("void doStuff(Test, Test)", asFUNCTION(doStuff), asCALL_CDECL);

	const char *script =
	"Test test1, test2;                \n"
	"doStuff( test1, test1 + test2 );  \n"  // This one will work
	"doStuff( test1 + test2, test1 );  \n"; // This one will blow

	r = ExecuteString(engine, script);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();
	return fail;
}

//-----------------------------------------
// Test 3 was reported by loboWu
bool Test3()
{
	bool fail = false;
	COutStream out;
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, true);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	const char *script =
	"uint8 search_no(uint8[]@ cmd, uint16 len, uint8[] @rcv)    \n" //mbno is nx5 array, static
	"{														    \n"
	"	if (@rcv == null) assert(false);						\n"
	"		return(255);										\n"
	"}															\n"
	"void main()												\n"
	"{															\n"
	"	uint8[] cmd = { 0x02, 0x95, 0x45, 0x42, 0x32 };			\n"
	"	uint8[] rcv;											\n"
	"	uint16 len = 8;											\n"
	"	search_no(cmd, cmd.length(), rcv);						\n" //This is OK! @rcv won't be null
	"	search_no(cmd, GET_LEN2(cmd), rcv);						\n" //This is OK!
	"	len = GET_LEN(cmd);										\n"
	"	search_no(cmd, len, rcv);								\n" //This is OK!
	"															\n"//but
	"	search_no(cmd, GET_LEN(cmd), rcv);						\n" //@rcv is null
	"}															\n"
	"uint16 GET_LEN(uint8[]@ cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n"
	"uint16 GET_LEN2(uint8[] cmd)								\n"
	"{															\n"
	"	return cmd[0]+3;										\n"
	"}															\n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();
	return fail;
}

//----------------------------------------
// Test 4 reported by dxj19831029
bool Test4()
{
	bool fail = false;
	COutStream out;
	int r = 0;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->RegisterObjectType("Chars", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_FACTORY, "Chars@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Chars", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectMethod("Chars", "Chars &opAssign(const Chars &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterObjectType("_Save", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectProperty("_Save", "Chars FieldName", 0);

	engine->RegisterObjectType("Struct", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_FACTORY, "Struct@ f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Struct", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_GENERIC);
	engine->RegisterObjectProperty("Struct", "_Save Save", 0);

	engine->RegisterObjectType("ScriptObject", 4, asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
	engine->RegisterObjectMethod("ScriptObject", "Struct @f()", asFUNCTION(0), asCALL_GENERIC);

	engine->RegisterGlobalProperty("ScriptObject current", (void*)1);

	engine->RegisterGlobalFunction("void print(Chars&)", asFUNCTION(0), asCALL_GENERIC);

	const char *script1 = "void main() { print(current.f().Save.FieldName); }";
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script1, strlen(script1));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	const char *script2 = "void main() { Chars a = current.f().Save.FieldName; print(a); }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	engine->Release();
	return fail;
}

//-----------------------------------------------
// Test5 reported by jal
bool Test5()
{
	// This script caused an assert failure during compilation
	const char *script =
		"class cFlagBase {} \n"
		"void CTF_getBaseForOwner( )   \n"
		"{  \n"
		"   for ( cFlagBase @flagBase; ; @flagBase = null ) \n"
		"   {  \n"
		"	}  \n"
		"}   ";

	bool fail = false;
	COutStream out;
	int r = 0;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	engine->SetEngineProperty(asEP_OPTIMIZE_BYTECODE, 0);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	engine->Release();
	return fail;
}

//-------------------------------------------------
// Test6 reported by SiCrane
bool Test6()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

	// The following script would enter an infinite loop while building
	const char *script1 =
		"class Foo { \n"
		"const int foo(int a) { return a; } \n"
		"} \n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// This should also work
	const char *script2 =
		"interface IFoo { \n"
		"	const int foo(int a); \n"
		"} \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// This would cause an assert failure
	const char *script3 =
		"class MyClass { \n"
		"    MyClass(int a) {} \n"
		"} \n"
		"const MyClass foo(int (a) ,bar); \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script3, strlen(script3));
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (4, 18) : Info    : Compiling const MyClass foo\n"
					   "script (4, 28) : Error   : 'bar' is not declared\n"
					   "script (4, 24) : Error   : 'a' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// This would also cause an assert failure
	const char *script4 =
		"void main() { \n"
		"  for (;i < 10;); \n"
		"} \n";

	bout.buffer = "";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script4, strlen(script4));
	r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "script (1, 1) : Info    : Compiling void main()\n"
					   "script (2, 9) : Error   : 'i' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

//---------------------------------------
// Test7 reported by Vicious
// http://www.gamedev.net/community/forums/topic.asp?topic_id=525467
bool Test7()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		return false;

	bool fail = false;

	const char *script =
	"void GENERIC_CommandDropItem( cClient @client )	\n"
	"{													\n"
	"	client.getEnt().health -= 1;					\n"
	"}													\n";

	// 1. tmp1 = client.getEnt()
	// 2. tmp2 = tmp1.health
	// 3. tmp3 = tmp2 - 1
	// 4. free tmp2
	// 5. tmp1.health = tmp3
	// 6. free tmp3
	// 7. free tmp1

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	COutStream out;
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	RegisterScriptString(engine);

	engine->RegisterObjectType("cEntity", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("cEntity", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("cEntity", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("cEntity", "cEntity @getEnt()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectProperty("cEntity", "int health", 0);
	engine->RegisterObjectType("cClient", 0, asOBJ_REF);
	engine->RegisterObjectBehaviour("cClient", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("cClient", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("cClient", "cEntity @getEnt()", asFUNCTION(0), asCALL_CDECL_OBJLAST);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	int r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

bool Test8()
{
	bool fail = false;

	CBufferedOutStream bout;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	RegisterScriptString(engine);

	// Must allow returning a const string
	const char *script = "const string func() { return ''; }";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script);
	int r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	r = ExecuteString(engine, "string str = func(); assert( str == '' );", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();

	return fail;
}

bool Test9()
{
	bool fail = false;
	CBufferedOutStream bout;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	const char *script = "void Func() \n"
						 "{ \n"
						 "	(aaa.AnyName())==0?1:0; \n"
						 "} \n";

	mod->AddScriptSection("sc", script);
	int r = mod->Build();
	if( r >= 0 )
		TEST_FAILED;

	if( bout.buffer != "sc (1, 1) : Info    : Compiling void Func()\n"
					   "sc (3, 3) : Error   : 'aaa' is not declared\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

//////////////////////////////
// AgentC reported problems
// http://www.gamedev.net/topic/614727-crash-with-temporary-value-types-and-unsafe-references/
// http://www.gamedev.net/topic/615762-assert-failures-of-unfreed-temp-variables/page__gopid__4887763#entry4887763

class Variant
{
public:
	Variant() {val = "test";}
	Variant(const Variant &other) {val = other.val;}
	~Variant() {val = "deleted";}
	Variant &operator=(const Variant &other) {return *this;}
	Variant &operator=(int v) {return *this;}
	const std::string &GetString() const {return val;}
	std::string val;
};

static void ConstructVariant(Variant *self)
{
	new(self) Variant();
}

static void ConstructVariantCopy(Variant &other, Variant *self)
{
	new(self) Variant(other);
}

static void DestructVariant(Variant *self)
{
	self->~Variant();
}

class VariantMap
{
public:
	Variant var;
	Variant &opIndex(const string &) { return var; }
};

static void ConstructVariantMap(VariantMap *self)
{
	new(self) VariantMap();
}

static void DestructVariantMap(VariantMap *self)
{
	self->~VariantMap();
}

class Node
{
public:
	Node() {refCount = 1;}
	void AddRef() {refCount++;}
	void Release() {if( --refCount == 0 ) delete this;}
	Variant GetAttribute() {return Variant();}
	int refCount;
	VariantMap vars;
};

static Node *NodeFactory()
{
	return new Node();
}

static Node *g_node = 0;
Node *GetGlobalNode()
{
	return g_node;
}

bool TestRetRef()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

	RegisterStdString(engine);

	engine->RegisterObjectType("Variant", sizeof(Variant), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("VariantMap", sizeof(VariantMap), asOBJ_VALUE | asOBJ_APP_CLASS_CD);
	engine->RegisterObjectType("Node", 0, asOBJ_REF);
#ifndef AS_MAX_PORTABILITY
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVariant), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_CONSTRUCT, "void f(const Variant&in)", asFUNCTION(ConstructVariantCopy), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructVariant), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("Variant", "const string& GetString() const", asMETHOD(Variant, GetString), asCALL_THISCALL);
	engine->RegisterObjectMethod("Variant", "Variant& opAssign(const Variant&in)", asMETHODPR(Variant, operator =, (const Variant&), Variant&), asCALL_THISCALL);
	engine->RegisterObjectMethod("Variant", "Variant& opAssign(int)", asMETHODPR(Variant, operator =, (int), Variant&), asCALL_THISCALL);

	engine->RegisterObjectBehaviour("VariantMap", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVariantMap), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("VariantMap", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructVariantMap), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectMethod("VariantMap", "Variant &opIndex(const string &in)", asMETHODPR(VariantMap, opIndex, (const string &), Variant&), asCALL_THISCALL);

	engine->RegisterObjectBehaviour("Node", asBEHAVE_FACTORY, "Node @f()", asFUNCTION(NodeFactory), asCALL_CDECL);
	engine->RegisterObjectBehaviour("Node", asBEHAVE_ADDREF, "void f()", asMETHOD(Node, AddRef), asCALL_THISCALL);
	engine->RegisterObjectBehaviour("Node", asBEHAVE_RELEASE, "void f()", asMETHOD(Node, Release), asCALL_THISCALL);
	engine->RegisterObjectMethod("Node", "Variant GetAttribute() const", asMETHODPR(Node, GetAttribute, (), Variant), asCALL_THISCALL);

	engine->RegisterGlobalFunction("Node@+ get_node()", asFUNCTION(GetGlobalNode), asCALL_CDECL);
#else
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(ConstructVariant), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_CONSTRUCT, "void f(const Variant&in)", WRAP_OBJ_LAST(ConstructVariantCopy), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Variant", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(DestructVariant), asCALL_GENERIC);
	engine->RegisterObjectMethod("Variant", "const string& GetString() const", WRAP_MFN(Variant, GetString), asCALL_GENERIC);
	engine->RegisterObjectMethod("Variant", "Variant& opAssign(const Variant&in)", WRAP_MFN_PR(Variant, operator =, (const Variant&), Variant&), asCALL_GENERIC);
	engine->RegisterObjectMethod("Variant", "Variant& opAssign(int)", WRAP_MFN_PR(Variant, operator =, (int), Variant&), asCALL_GENERIC);

	engine->RegisterObjectBehaviour("VariantMap", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(ConstructVariantMap), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("VariantMap", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(DestructVariantMap), asCALL_GENERIC);
	engine->RegisterObjectMethod("VariantMap", "Variant &opIndex(const string &in)", WRAP_MFN_PR(VariantMap, opIndex, (const string &), Variant&), asCALL_GENERIC);

	engine->RegisterObjectBehaviour("Node", asBEHAVE_FACTORY, "Node @f()", WRAP_FN(NodeFactory), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Node", asBEHAVE_ADDREF, "void f()", WRAP_MFN(Node, AddRef), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("Node", asBEHAVE_RELEASE, "void f()", WRAP_MFN(Node, Release), asCALL_GENERIC);
	engine->RegisterObjectMethod("Node", "Variant GetAttribute() const", WRAP_MFN_PR(Node, GetAttribute, (), Variant), asCALL_GENERIC);

	engine->RegisterGlobalFunction("Node@+ get_node()", WRAP_FN(GetGlobalNode), asCALL_GENERIC);
#endif
	engine->RegisterObjectProperty("Node", "VariantMap vars", asOFFSET(Node, vars));

	g_node = NodeFactory();

	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", "void f() { \n"
								  "  string str = node.GetAttribute().GetString(); // Get its first attribute as a string \n"
								  "  assert( str == 'test' ); \n"
								  "} \n");
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	r = ExecuteString(engine, "f();", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// Test for assert failures
	mod->AddScriptSection("test", "class GameObject { \n"
		                          "  int health; \n"
								  "  GameObject() { \n"
								  "    health = 10; \n"
                                  "  } \n"
								  "  void Update(float deltaTime) { \n"
								  "    node.vars['Health'] = health; \n" // This will cause unfreed temp variable of type Node@
                                  "  } \n"
                                  "} \n");
	bout.buffer = "";
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();
#ifndef AS_MAX_PORTABILITY
	// When using the generic calling convention, the returned handle is not incremented
	g_node->Release();
#endif

	return fail;
}

} // namespace

