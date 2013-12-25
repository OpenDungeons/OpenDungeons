#include "utils.h"
#include "../../add_on/scripthandle/scripthandle.h"
#include "../../add_on/scriptmath/scriptmathcomplex.h"

namespace TestScriptStruct
{

static const char * const TESTNAME = "TestScriptStruct";

// Normal structure
static const char *script1 =
"class Test                   \n"
"{                            \n"
"   int a;                    \n"
"   bool b;                   \n"
"};                           \n"
"void TestStruct()            \n"
"{                            \n"
"   Test a;                   \n"
"   a.a = 3;                  \n"
"   a.b = false;              \n"
"   Test b;                   \n"
"   Test @c = @a;             \n"
"   a = b;                    \n"
"   TestStruct2(c);           \n"
"   Test[] d(1);              \n"
"   d[0] = a;                 \n"
"   a = Test();               \n"
"}                            \n"
"void TestStruct2(Test a)     \n"
"{                            \n"
"}                            \n";

// Do not allow const in properties
static const char *script2 = 
"class Test                   \n"
"{                            \n"
"   const int a;              \n"
"};                           \n";

// Test arrays in struct
static const char *script3 =
"class Test                   \n"
"{                            \n"
"   int[] a;                  \n"
"};                           \n"
"class Test2                  \n"
"{                            \n"
"   Test2@[][] a;             \n"
"};                           \n"
"void TestArrayInStruct()     \n"
"{                            \n"
"   Test a;                   \n"
"   a.a.resize(10);           \n"
"   Test2 b;                  \n"
"   b.a.resize(1);            \n"
"   b.a[0].resize(1);         \n"
"   // Circular reference     \n"
"   @b.a[0][0] = b;           \n"
"}                            \n";

// Only allow primitives (at first)
static const char *script4 =
"class B                      \n"
"{                            \n"
"   A a;                      \n"
"   string b;                 \n"
"   int c;                    \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"  B a, b;                    \n"
"  b.a.a = 5;                 \n"
"  b.b = \"Test\";            \n"
"  b.c = 6;                   \n"
"  a = b;                     \n"
"  b.a.a = 6;                 \n"
"  b.b = \"1\";               \n"
"  b.c = 2;                   \n"
"  Assert(a.a.a == 5);        \n"
"  Assert(a.b == \"Test\");   \n"
"  Assert(a.c == 6);          \n"
"}                            \n"
"class A                      \n"
"{                            \n"
"   uint a;                   \n"
"};                           \n";

// Verify that the struct names cannot conflict with one another
static const char *script5 = 
"class A {};                  \n"
"class A {};                  \n"
"class B                      \n"
"{                            \n"
"  int a;                     \n"
"  float a;                   \n"
"};                           \n";

// Verify that a structure cannot have itself as local member (directly or indirectly)
static const char *script6 = 
"class A                      \n"
"{                            \n"
"  A a;                       \n"
"};                           \n"
"class B                      \n"
"{                            \n"
"  C c;                       \n"
"};                           \n"
"class C                      \n"
"{                            \n"
"  B b;                       \n"
"};                           \n";

static const char *script7 =
"class A                      \n"
"{                            \n"
"  string@ s;                 \n"
"};                           \n"
"void TestHandleInStruct()    \n"
"{                            \n"
"  A a;                       \n"
"  Assert(@a.s == null);      \n"
"  a = a;                     \n"
"  @a.s = \"Test\";           \n"
"  Assert(a.s == \"Test\");   \n"
"}                            \n";

// Verify that circular references are handled by the GC
static const char *script8 = 
"class A                      \n"
"{                            \n"
"  A@ next;                   \n"
"};                           \n"
"class B                      \n"
"{                            \n"
"  D@ next;                   \n"
"};                           \n"
"class C                      \n"
"{                            \n"
"  B b;                       \n"
"};                           \n"
"class D                      \n"
"{                            \n"
"  C c;                       \n"
"};                           \n"
"void TestHandleInStruct2()   \n"
"{                            \n"
// Simple circular reference
"  A a;                       \n"
"  @a.next = a;               \n"
// More complex circular reference
"  D d1;                      \n"
"  D d2;                      \n"
"  @d1.c.b.next = d2;         \n"
"  @d2.c.b.next = d1;         \n"
"}                            \n";


static const char *script9 = 
"class MyStruct               \n"
"{                            \n"
"  uint myBits;               \n"
"};                           \n"
"uint MyFunc(uint a)          \n"
"{                            \n"
"  return a;                  \n"
"}                            \n"
"void MyFunc(string@) {}      \n"
"void Test()                  \n"
"{                            \n"
"  uint val = 0x0;            \n"
"  MyStruct s;                \n"
"  s.myBits = 0x5;            \n"
"  val = MyFunc(s.myBits);    \n"
"}                            \n";

// Don't allow arrays of the struct type as members (unless it is handles)
static const char *script10 = 
"class Test2                  \n"
"{                            \n"
"   Test2[] a;                \n"
"};                           \n";

// Test array constness in members
static const char *script11 = 
"class A                      \n"
"{                            \n"
"   int[] a;                  \n"
"};                           \n"
"void Test()                  \n"
"{                            \n"
"   const A a;                \n"
"   // Should not compile     \n"
"   a.a[0] = 23;              \n"
"}                            \n";

// Test order independence with declarations
static const char *script12 =
"A Test()                     \n"
"{                            \n"
"  A a;                       \n"
"  return a;                  \n"
"}                            \n";

static const char *script13 =
"class A                      \n"
"{                            \n"
"  B b;                       \n"
"};                           \n"
"class B                      \n"
"{                            \n"
"  int val;                   \n"
"};                           \n";

static const char *script14 =
"class A                     \n"
"{                           \n"
"  B @b;                     \n"
"}                           \n"
"class B                     \n"
"{                           \n"
"  int val;                  \n"
"}                           \n";


bool Test2();

void TraceExec(asIScriptContext *ctx, void *)
{
//	for( asUINT n = 0; n < ctx->GetCallstackSize(); n++ )
//		printf(" ");
//	printf("%s", ctx->GetFunction()->GetDeclaration());
//	printf("    Line: %d\n", ctx->GetLineNumber());
}

bool Test()
{
	bool fail = false;
	int r;
	asIScriptModule *mod;
	asIScriptEngine *engine;
	COutStream out;
	CBufferedOutStream bout;

	// Test class containing array with default size
	// http://www.gamedev.net/topic/640059-crash-class-and-array-with-initial-size/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterScriptArray(engine, true);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "class A{ \n"
									  "  A(){} \n"
									  "  float[] cts(12); \n"
									  "  float[] b; \n"
									  "}; \n"
									  "A a; \n");
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

	// CreateScriptObject should give proper error when attempting call for class without default constructor
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "class A { A(int) {} }");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIObjectType *type = mod->GetObjectTypeByName("A");
		if( engine->CreateScriptObject(type->GetTypeId()) )
			TEST_FAILED;

		if( bout.buffer != " (0, 0) : Error   : Failed in call to function 'CreateScriptObject' (Code: -6)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Properly report error when using member initialization expressions
	// http://www.gamedev.net/topic/638613-asassert-in-file-as-compillercpp-line-675/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"enum SomeEnum \n"
		    "{ \n"
			"  en_A \n"
			"} \n"
			"int GetVal( SomeEnum some ){ \n"
			"  return 0; \n"
			"} \n"
			"class B \n"
			"{ \n"
			"  int some_val = GetVal( en_B ); \n"
			"} \n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (8, 7) : Info    : Compiling B::B()\n"
                           "test (10, 26) : Error   : 'en_B' is not declared\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test a problem reported by Andrew Ackermann
	// Null pointer access exception in constructor due to access of members before they have been initialized
	// TODO: decl: Members that are initialized with trivial expressions, i.e. don't call members 
	//             of the class, should be initialized before the base class. This will make it even
	//             less likely for a base class to access uninitialized members in a derived class
	// TODO: decl: It might be possible to do a static code analysis if any function calls are made 
	//             before the call to super() and then check if those functions access the members. 
	//             This is quite complex and won't be implemented now.
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		const char *script =
			"interface IGuiElement { \n"
			"  void addChild(IGuiElement @e); \n"
			"} \n"
			"class BaseGuiElement : IGuiElement { \n"
			"  BaseGuiElement(IGuiElement @e) { \n"
			"    _BaseGuiElement(e); \n"
			"  } \n"
			"  void _BaseGuiElement(IGuiElement @e) { \n"
			"    @parent = e; \n"
			"  } \n"
			"  void set_parent(IGuiElement @e) { \n"
			"    if( e !is null ) \n"
			"      e.addChild(this); \n"
			"  } \n"
			"  void addChild(IGuiElement @e) { \n"
			"    Children.insertLast(e); \n"
			"  } \n"
			"  IGuiElement@[] Children; \n"
			"} \n"
			"class GuiButton : BaseGuiElement { \n"
			"  GuiButton(IGuiElement @e) { \n"
			"    super(e); \n"
			"  } \n"
			"} \n"
			"class GuiScrollBar : BaseGuiElement { \n"
			"  GuiButton @button; \n"
			"  GuiScrollBar(IGuiElement @e) { \n"
			"    @button = GuiButton(this); \n" // The construction of the GuiButton tries to access members that are only initialized with super(e)
			"    super(e); \n"                  // Changing the code to call super(e) first resolves the exception, but this wasn't necessary in version 2.25.2
			"  } \n"
			"} \n";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->SetLineCallback(asFUNCTION(TraceExec), 0, asCALL_CDECL);
		r = ExecuteString(engine, "BaseGuiElement base(null); GuiScrollBar scroll(base);", mod, ctx);
		if( strstr(asGetLibraryOptions(), "AS_NO_MEMBER_INIT") )
		{
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
		}
		else
		{
			if( r != asEXECUTION_EXCEPTION )
				TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	// Test a problem reported by Andrew Ackermann
	// The compiler didn't set the correct stack size for the constructor so it ended up corrupting the memory
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		const char *script =
			"shared class PowerTargeting { \n"
			"}; \n"
			"shared class EnergyPower { \n"
			"	PowerTargeting targeting; \n"
			"	PowerTargeting[] objectEffects; \n"
			"	EnergyPower() { \n"
			"	} \n"
			"}; \n";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "EnergyPower e;", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test a problem reported by Andrew Ackermann
	// Before the change to support initialization directly in member declaration, all members were guaranteed
	// to be initialized before the code in the constructor begun. The solution to the below was to initialize
	// members that had no initialization expression in the beginning of the constructor even if the base class
	// constructor is explicitly called.
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"class Base { \n"
			"  Base() { SetMember(); } \n"
			"  void SetMember() {} \n"
			"}; \n"
			"class Derived : Base { \n"
			"   string member; \n"
			"	Derived() { \n"
			"     super(); \n" // Explicitly call base class' constructor, which makes members be initialized after
			"	} \n"
			// Override base class SetMember method
			"   void SetMember() { \n"
			"     member = 'hello'; \n"
			"   } \n"
			"}; \n";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "Derived d; assert( d.member == 'hello' );", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test a problem reported by Andre Santee
	// Inheriting from a base class without default constructor and no explicit call to super should give compiler error
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		const char *script =
			"class Foo \n"
			"{ \n"
			"  Foo(int a) \n"
			"  { \n"
			"  } \n"
			"} \n"
			"class Bar : Foo \n"
			"{ \n"
			"  void func() \n"
			"  { \n"
			"  } \n"
			"} \n"
			"class Bar2 : Foo \n"
			"{ \n"
			"  Bar2() { } \n"
			"} \n";

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
 		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (7, 7) : Info    : Compiling Bar::Bar()\n"
						   "test (7, 7) : Error   : Base class doesn't have default constructor. Make explicit call to base constructor\n"
						   "test (15, 3) : Info    : Compiling Bar2::Bar2()\n"
						   "test (15, 10) : Error   : Base class doesn't have default constructor. Make explicit call to base constructor\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	if( !strstr(asGetLibraryOptions(), "AS_NO_MEMBER_INIT") )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterScriptArray(engine, true);
		RegisterStdString(engine);
		RegisterScriptHandle(engine);
		RegisterScriptMathComplex(engine);

		// Null pointer exception when attempting to access an object before it has been initialized
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { \n"
			"  string hello = 'hello'; \n"
			"  int a = Func(); \n"
			"  string str = 'again'; \n"
			"  int Func() { return str.length; } \n"
			"}");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj != 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "T t;", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		if( r == asEXECUTION_EXCEPTION && std::string(ctx->GetExceptionString()) != "Null pointer access" )
		{
			printf("%s\n", ctx->GetExceptionString());
			TEST_FAILED;
		}
		if( std::string(ctx->GetExceptionFunction()->GetName()) != "Func" )
			TEST_FAILED;
		if( std::string(ctx->GetFunction(1)->GetName()) != "T" )
			TEST_FAILED;
		if( ctx->GetLineNumber(1) != 3 )
			TEST_FAILED;
		ctx->Release();
		
		// Default initialization of object members without initialization expression
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { \n"
			"  string a; \n"
			"  array<int> b; \n"
			"  ref c; \n"
			"  E e; \n"
			"  FUNC @f; \n"
			"} \n"
			"enum E { EVAL } \n"
			"funcdef void FUNC(); \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			if( *reinterpret_cast<std::string*>(obj->GetAddressOfProperty(0)) != "" )
				TEST_FAILED;
			if( reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(1))->GetElementTypeId() != asTYPEID_INT32 )
				TEST_FAILED;
			if( !reinterpret_cast<CScriptHandle*>(obj->GetAddressOfProperty(2))->Equals(0,0) )
				TEST_FAILED;
		}

		if( obj )
			obj->Release();

		// Initialize array of classes in shared class
		const char *script = "shared class MyClass {} \n"
			"shared class T { \n"
			"  array<MyClass> a; \n"
			"  MyClass[] b; \n"
			"  array<MyClass> c = {MyClass()}; \n"
			"  array<MyClass@> d = {MyClass()}; \n"
			"  array<array<MyClass>> e = {{MyClass()}, {MyClass()}}; \n"
			"} \n";
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		mod = engine->GetModule("test2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			CScriptArray *arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(0));
			if( arr == 0 )
				TEST_FAILED;
			else
			{
				if( arr->GetElementTypeId() != mod->GetTypeIdByDecl("MyClass") )
					TEST_FAILED;
				arr->Resize(1);
			}

			arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(1));
			if( arr == 0 )
				TEST_FAILED;
			else
			{
				if( arr->GetElementTypeId() != mod->GetTypeIdByDecl("MyClass") )
					TEST_FAILED;
				arr->Resize(1);
			}

			arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(2));
			if( arr == 0 )
				TEST_FAILED;
			else
			{
				if( arr->GetElementTypeId() != mod->GetTypeIdByDecl("MyClass") )
					TEST_FAILED;
				if( arr->GetSize() != 1 )
					TEST_FAILED;
			}

			arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(3));
			if( arr == 0 )
				TEST_FAILED;
			else
			{
				if( arr->GetElementTypeId() != mod->GetTypeIdByDecl("MyClass@") )
					TEST_FAILED;
				if( arr->GetSize() != 1 )
					TEST_FAILED;
			}
		}

		if( obj )
			obj->Release();

		// Initialization from mixin classes (in same script)
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"mixin class M { \n"
			"  array<int> a = {1,2,3}; \n"
			"  int c = b*2; \n"
			"} \n"
			"class T : M { \n"
			"  int b = 12; \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			CScriptArray *arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(1));
			if( arr->GetElementTypeId() != asTYPEID_INT32 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(0)) != 1 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(1)) != 2 )
				TEST_FAILED;
			
			if( *reinterpret_cast<int*>(obj->GetAddressOfProperty(0)) != 12 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(obj->GetAddressOfProperty(2)) != 24 )
				TEST_FAILED;
		}

		if( obj )
			obj->Release();

		// Initialization from mixin classes (in other script)
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"mixin class M { \n"
			"  array<int> a = {1,2,b.length()}; \n" // provoke exception by accessing b before it is initialized
			"  string b = 'hello'; \n"
			"} \n");
		mod->AddScriptSection("test2",
			"class T : M { \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// test saving/loading bytecode with mixin class from different file
		CBytecodeStream stream("test");
		r = mod->SaveByteCode(&stream);
		if( r < 0 ) 
			TEST_FAILED;
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "T t;", mod, ctx);
		if( r != asEXECUTION_EXCEPTION )
			TEST_FAILED;
		const char *section = 0;
		int line = ctx->GetExceptionLineNumber(0, &section);
		if( line != 2 || std::string(section) != "test" )
			TEST_FAILED;
		ctx->Release();


		// Test compiler error when including mixin from other script
		bout.buffer = "";
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"mixin class M { \n"
			"  int a = func(); \n"
			"} \n");
		mod->AddScriptSection("test2",
			"class T : M { \n"
			"} \n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "test2 (1, 7) : Info    : Compiling T::T()\n"
		                   "test (2, 11) : Error   : No matching signatures to 'func()'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		// Explicit initialization of object members
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { \n"
			"  string a = 'hello'; \n"
			"  array<int> b = {1,2,3}; \n"
			"  ref @c = @b; \n"
			"  complex d(1,2); \n"
			"  E e = EVAL; \n"
			"  FUNC @f = func; \n"
			"} \n"
			"enum E { EVAL = 42 }\n"
			"funcdef void FUNC(); \n"
			"void func() {} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			if( *reinterpret_cast<std::string*>(obj->GetAddressOfProperty(0)) != "hello" )
				TEST_FAILED;
			CScriptArray *arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(1));
			if( arr->GetElementTypeId() != asTYPEID_INT32 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(0)) != 1 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(1)) != 2 )
				TEST_FAILED;
			CScriptHandle *ref = reinterpret_cast<CScriptHandle*>(obj->GetAddressOfProperty(2));
			if( !ref->Equals(&arr, engine->GetTypeIdByDecl("array<int> @")) )
				TEST_FAILED;
			Complex *cmplx = reinterpret_cast<Complex*>(obj->GetAddressOfProperty(3));
			if( cmplx->r != 1 || cmplx->i != 2 )
				TEST_FAILED;
			asUINT e = *reinterpret_cast<asUINT*>(obj->GetAddressOfProperty(4));
			if( e != 42 )
				TEST_FAILED;
			asIScriptFunction *func = *reinterpret_cast<asIScriptFunction**>(obj->GetAddressOfProperty(5));
			if( std::string(func->GetName()) != "func" )
				TEST_FAILED;
		}

		if( obj )
			obj->Release();

		// Test creating script class instance without initialization (for serialization)
		obj = (asIScriptObject*)engine->CreateUninitializedScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			if( *reinterpret_cast<std::string*>(obj->GetAddressOfProperty(0)) != "" )
				TEST_FAILED;
			CScriptArray *arr = reinterpret_cast<CScriptArray*>(obj->GetAddressOfProperty(1));
			if( arr->GetElementTypeId() != asTYPEID_INT32 )
				TEST_FAILED;
			if( arr->GetSize() != 0 )
				TEST_FAILED;
			CScriptHandle *ref = reinterpret_cast<CScriptHandle*>(obj->GetAddressOfProperty(2));
			if( !ref->Equals(0, 0) )
				TEST_FAILED;
			Complex *cmplx = reinterpret_cast<Complex*>(obj->GetAddressOfProperty(3));
			if( cmplx->r != 0 || cmplx->i != 0 )
				TEST_FAILED;
		}
		if( obj )
			obj->Release();

		// Default initialization of primitive members
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { int a = 42, b = a/2; }");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			if( *reinterpret_cast<int*>(obj->GetAddressOfProperty(0)) != 42 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(obj->GetAddressOfProperty(1)) != 21 )
				TEST_FAILED;
		}

		if( obj )
			obj->Release();

		// Errors must be reported on the correct line
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		mod->AddScriptSection("test",
			"class T { \n"
			"  int a = 0/0; \n"
			"  T() {} \n"
			"}");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "test (3, 3) : Info    : Compiling T::T()\n"
		                   "test (2, 12) : Error   : Divide by zero\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Initialization of handle members
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class T { \n"
			" array<int> @a = newArray(); \n"
			" array<int> @b = a; \n"
			// TODO: the init list should work for handles too
			" array<int> @newArray() { array<int> a = {1,2,3}; return a; } \n"
			"}");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		obj = (asIScriptObject*)engine->CreateScriptObject(mod->GetTypeIdByDecl("T"));
		if( obj == 0 )
			TEST_FAILED;
		else
		{
			CScriptArray *arr = *reinterpret_cast<CScriptArray**>(obj->GetAddressOfProperty(0));
			if( arr->GetSize() != 3 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(0)) != 1 )
				TEST_FAILED;
			if( *reinterpret_cast<int*>(arr->At(1)) != 2 )
				TEST_FAILED;
			CScriptArray *arrB = *reinterpret_cast<CScriptArray**>(obj->GetAddressOfProperty(1));
			if( arr != arrB )
				TEST_FAILED;
		}

		if( obj )
			obj->Release();

		// Expressions are evaluated in the scope where they will be executed
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Base { Base(int _a) {} int a = _a; } \n"
			"class T : Base { T(int _a) { if( _a == 0 ) super(42); else super(24); } int b = a; } \n"
			"void test() { \n"
			"  T a(0); \n"
			"  assert( a.b == 42 ); \n"
			"  T b(1); \n"
			"  assert( b.b == 24 ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "test()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, true);
		RegisterScriptString(engine);

		engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		// Test declaring multiple properties in same declaration separated by ,
		{
			mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
			mod->AddScriptSection(TESTNAME, 
				"class A { \n"
				"  int a, b, c; \n"
				"  void f() { a = b = c; } \n"
				"} \n");

			r = mod->Build();
			if( r < 0 ) TEST_FAILED;
		}


		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script1);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		// Verify that GetObjectTypeByIndex recognizes the script class
		if( mod->GetObjectTypeCount() != 1 )
			TEST_FAILED;
		asIObjectType *type = mod->GetObjectTypeByIndex(0);
		if( strcmp(type->GetName(), "Test") != 0 )
			TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "TestStruct()", mod, ctx);
		if( r != asEXECUTION_FINISHED ) 
		{
			if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
			TEST_FAILED;
		}
		if( ctx ) ctx->Release();

		bout.buffer = "";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(TESTNAME, script2);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		r = mod->Build();
		if( r >= 0 || bout.buffer != "TestScriptStruct (3, 4) : Error   : Class properties cannot be declared as const\n" ) TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script3);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "TestArrayInStruct()", mod);
		if( r != 0 ) TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script4, strlen(script4), 0);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if( r != 0 ) TEST_FAILED;

		bout.buffer = "";
		mod->AddScriptSection(TESTNAME, script5, strlen(script5), 0);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		r = mod->Build();
		if( r >= 0 || bout.buffer != 
			"TestScriptStruct (2, 7) : Error   : Name conflict. 'A' is a class.\n"
			"TestScriptStruct (6, 9) : Error   : Name conflict. 'a' is an object property.\n" ) TEST_FAILED;

		bout.buffer = "";
		mod->AddScriptSection(TESTNAME, script6, strlen(script6), 0);
		r = mod->Build();
		if( r >= 0 || bout.buffer !=
			"TestScriptStruct (1, 7) : Error   : Illegal member type\n"
			"TestScriptStruct (5, 7) : Error   : Illegal member type\n" ) TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script7, strlen(script7), 0);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		ctx = engine->CreateContext();
		r = ExecuteString(engine, "TestHandleInStruct()", mod, ctx);
		if( r != 0 )
		{
			if( r == asEXECUTION_EXCEPTION )
			{
				printf("%s\n", ctx->GetExceptionString());
			}
			TEST_FAILED;
		}
		if( ctx ) ctx->Release();

		mod->AddScriptSection(TESTNAME, script8, strlen(script8), 0);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "TestHandleInStruct2()", mod);
		if( r != 0 ) TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script9, strlen(script9), 0);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "Test()", mod);
		if( r != 0 ) TEST_FAILED;

		bout.buffer = "";
		mod->AddScriptSection(TESTNAME, script10, strlen(script10), 0);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		r = mod->Build();
		if( r >= 0 ) TEST_FAILED;
		if( bout.buffer != "TestScriptStruct (1, 7) : Error   : Illegal member type\n" ) TEST_FAILED;

		bout.buffer = "";
		mod->AddScriptSection(TESTNAME, script11, strlen(script11), 0);
		r = mod->Build();
		if( r >= 0 ) TEST_FAILED;
		if( bout.buffer != "TestScriptStruct (5, 1) : Info    : Compiling void Test()\nTestScriptStruct (9, 11) : Error   : Reference is read-only\n" ) TEST_FAILED;

		mod->AddScriptSection(TESTNAME, script12, strlen(script12), 0);
		mod->AddScriptSection(TESTNAME, script13, strlen(script13), 0);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		// The garbage collection doesn't have to be invoked immediately. Modules
		// can even be discarded before calling the garbage collector.
		engine->GarbageCollect();
		
		// Make sure it is possible to copy a script class that contains an object handle
		mod->AddScriptSection(TESTNAME, script14, strlen(script14), 0);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "A a; B b; @a.b = @b; b.val = 1; A a2; a2 = a; Assert(a2.b.val == 1);", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// Make sure it is possible to copy a script class that contains an array
		const char *script15 = 
			"class Some \n"
			"{ \n"
			"    int[] i; // need be array \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"    Some e; \n"
			"    e=some(e); // crash \n"
			"} \n"
			"Some@ some(Some@ e) \n"
			"{ \n"
			"    return e; \n"
			"} \n";

		mod->AddScriptSection(TESTNAME, script15);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// A script class must be able to have a registered ref type as a local member
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);

		const char *script = "class C { string s; }";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);

		bout.buffer = "";
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		if( !fail )
		{
			r = ExecuteString(engine, "C c; c.s = 'test';", mod);
			if( r != asEXECUTION_FINISHED )
			{
				TEST_FAILED;
			}
		}

		engine->Release();
	}

	// Test private properties
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		const char *script = "class C { \n"
			                 "private int a; \n"
							 "void func() { a = 1; } }\n"
							 "void main() { C c; c.a = 2; }";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 ) 
			TEST_FAILED;

		if( bout.buffer != "s (4, 1) : Info    : Compiling void main()\n"
		                   "s (4, 21) : Error   : Illegal access to private property 'a'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test private methods
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script = "class C { \n"
							 "  private void func() {} \n"
							 "  void func2() { func(); } } \n"
							 "void main() { C c; c.func(); } \n";
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "s (4, 1) : Info    : Compiling void main()\n"
		                   "s (4, 20) : Error   : Illegal call to private method 'void C::func()'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test private methods that return a value
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = "class C { \n"
			"  int a(int i) { return ABS(i); } \n"
			"  private int ABS(int i) \n"
			"  { \n"
			"    if(i <= 0) return (-1 * i); \n"
			"    else return i; \n"
			"  } \n"
			"} \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("s", script);

		bout.buffer = "";
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		r = ExecuteString(engine, "C c; assert( c.a(-10) == 10 );", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test private methods with inheritance
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"bool alreadyCalled = false; \n"
			"class CBar \n"
			"{ \n"
			" CBar() \n"
			" { \n"
			"  assert(alreadyCalled == false); \n"
			"  alreadyCalled = true; \n"
			" } \n"
			" void Foo() \n"
			" { \n"
			" } \n"
			"}; \n"
			"class CDerivedBar : CBar \n"
			"{ \n"
			" CDerivedBar() \n"
			" { \n"
			" } \n"
			" private void ImNotAnOverrideOfTheBaseClass() \n"
			" { \n"
			" } \n"
			" private void Foo() \n" 
			" { \n"
			" } \n"
			"}; \n";

		asIScriptModule *mod = engine->GetModule("t", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		mod->Build();

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "CDerivedBar bar; bar.Foo(); ", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PrintException(ctx, true);
			TEST_FAILED;
		}
		ctx->Release();
 
		engine->Release();
	}

	// Default constructor and opAssign shouldn't be provided if a non-default constructor is implemented
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		const char *script = 
			"class CBar \n"
			"{ \n"
			" CBar(int a) {}\n"
			"}; \n"
			"void func() \n"
			"{ \n"
			"  CBar a; \n" // not ok
			"  CBar b(1); \n" // ok
			"  CBar c = CBar(1); \n" // not ok
			"  b = b; \n" // not ok
			"  CBar d(CBar()); \n" // not ok
			"}; \n";

		asIScriptModule *mod = engine->GetModule("t", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);

		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (5, 1) : Info    : Compiling void func()\n"
						   "script (7, 8) : Error   : No default constructor for object of type 'CBar'.\n"
						   "script (9, 8) : Error   : No default constructor for object of type 'CBar'.\n"
						   "script (9, 8) : Error   : There is no copy operator for the type 'CBar' available.\n"
						   "script (10, 5) : Error   : There is no copy operator for the type 'CBar' available.\n"
						   "script (11, 10) : Error   : No matching signatures to 'CBar()'\n"
						   "script (11, 10) : Info    : Candidates are:\n"
						   "script (11, 10) : Info    : CBar@ CBar(int)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
 
		engine->Release();
	}

	fail = Test2() || fail;

	// Success
	return fail;
}


//--------------------------------
// Test reported by SiCrane
// 
// Doing an assignment of a temporary object would give an incorrect result, even crashing the application
bool Test2()
{
	bool fail = false;
	COutStream out;
	int r;


	const char *script1 = 
		"class MyClass {                  \n"
		"  int a;                         \n"
		"  MyClass(int a) { this.a = a; } \n"
		"  MyClass &opAssign(const MyClass &in o) { a = o.a; return this; } \n"
		"  int foo() { return a; }        \n"
		"}                                \n"
		"                                 \n"
		"void main() {                    \n"
		"  int i;                         \n"
		"  MyClass m(5);                  \n"
		"  MyClass t(10);                 \n"
		"  i = (m = t).a;                 \n"
		"  assert(i == 10);               \n"
		"  i = (m = MyClass(10)).a;       \n"
		"  assert(i == 10);               \n"
		"  MyClass n(10);                 \n"
		"  MyClass o(15);                 \n"
		"  m = n = o;                     \n"
		"  m = n = MyClass(20);           \n"
		"  (m = n).foo();                 \n"
		"  (m = MyClass(20)).foo();       \n"
		"}                                \n";

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

} // namespace

