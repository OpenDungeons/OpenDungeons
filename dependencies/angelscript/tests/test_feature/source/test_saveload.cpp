//
// Tests importing functions from other modules
//
// Test author: Andreas Jonsson
//

#include <memory>
#include <vector>
#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"


namespace TestSaveLoad
{

using namespace std;

static const char * const TESTNAME = "TestSaveLoad";



static const char *script1 =
"import void Test() from 'DynamicModule';     \n"
"OBJ g_obj;                                   \n"
"A @gHandle;                                  \n"
"funcdef void func_t(OBJ, float, A @);        \n"
"void func(OBJ o, float f, A @a) {}           \n"
"enum ETest {}                                \n"
"void main()                                  \n"
"{                                            \n"
"  Test();                                    \n"
"  TestStruct();                              \n"
"  TestArray();                               \n"
"  GlobalCharArray.resize(1);                 \n"
"  string @s = ARRAYTOHEX(GlobalCharArray);   \n"
"  func_t @f = func;                          \n"
"  f(OBJ(), 1, A());                          \n"
"}                                            \n"
"void TestObj(OBJ &out obj)                   \n"
"{                                            \n"
"}                                            \n"
"void TestStruct()                            \n"
"{                                            \n"
"  A a;                                       \n"
"  a.a = 2;                                   \n"
"  A@ b = @a;                                 \n"
"}                                            \n"
"void TestArray()                             \n"
"{                                            \n"
"  A[] c(3);                                  \n"
"  int[] d(2);                                \n"
"  A[]@[] e(1);                               \n"
"  @e[0] = @c;                                \n"
"}                                            \n"
"class A                                      \n"
"{                                            \n"
"  int a;                                     \n"
"  ETest e;                                   \n"
"};                                           \n"
"void TestHandle(string @str)                 \n"
"{                                            \n"
"}                                            \n"
"interface MyIntf                             \n"
"{                                            \n"
"  void test();                               \n"
"}                                            \n"
"class MyClass : MyIntf                       \n"
"{                                            \n"
"  array<int> arr = {1000,200,40,1};          \n"
"  int sum = arr[0] + arr[1] + arr[2] + arr[3]; \n"
"  void test() {number = sum;}                \n"
"}                                            \n";

static const char *script2 =
"void Test()                               \n"
"{                                         \n"
"  int[] a(3);                             \n"
"  a[0] = 23;                              \n"
"  a[1] = 13;                              \n"
"  a[2] = 34;                              \n"
"  if( a[0] + a[1] + a[2] == 23+13+34 )    \n"
"    number = 1234567890;                  \n"
"}                                         \n";

static const char *script3 = 
"float[] f(5);       \n"
"void Test(int a) {} \n";

static const char *script4 = 
"class CheckCollision                          \n"
"{                                             \n"
"	Actor@[] _list1;                           \n"
"                                              \n"
"	void Initialize() {                        \n"
"		_list1.resize(1);                      \n"
"	}                                          \n"
"                                              \n"
"	void Register(Actor@ entity){              \n"
"		@_list1[0] = @entity;                  \n"
"	}                                          \n"
"}                                             \n"
"                                              \n"
"CheckCollision g_checkCollision;              \n"
"                                              \n"
"class Shot : Actor {                          \n"
"	void Initialize(int a = 0) {               \n"
"		g_checkCollision.Register(this);       \n"
"	}                                          \n"
"}                                             \n"
"interface Actor {  }				           \n"
"InGame g_inGame;                              \n"
"class InGame					   	           \n"
"{									           \n"
"	Ship _ship;						           \n"
"	void Initialize(int level)		           \n"
"	{								           \n"
"		g_checkCollision.Initialize();         \n"
"		_ship.Initialize();	                   \n"
"	}						   		           \n"
"}									           \n"
"class Ship : Actor							   \n"
"{											   \n"
"   Shot@[] _shots;							   \n"
"	void Initialize()						   \n"
"	{										   \n"
"		_shots.resize(5);					   \n"
"                                              \n"
"		for (int i=0; i < 5; i++)              \n"
"		{                                      \n"
"			Shot shot;						   \n"
"			@_shots[i] = @shot;                \n"
"			_shots[i].Initialize();	           \n"
"		}                                      \n"
"	}										   \n"
"}											   \n";

// Make sure the handle can be explicitly taken for class properties, array members, and global variables
static const char *script5 =
"IsoMap      _iso;                                      \n"
"IsoSprite[] _sprite;                                   \n"
"                                                       \n"
"int which = 0;                                         \n"
"                                                       \n"
"bool Initialize() {                                    \n"
"  if (!_iso.Load('data/iso/map.imp'))                  \n"
"    return false;                                      \n"
"                                                       \n"
"  _sprite.resize(100);                                 \n"
"                                                       \n"
"  if (!_sprite[0].Load('data/iso/pacman.spr'))         \n"
"    return false;                                      \n"
"                                                       \n"
"  for (int i=1; i < 100; i++) {                        \n"
"    if (!_sprite[i].Load('data/iso/residencia1.spr'))  \n"
"      return false;                                    \n"
"  }                                                    \n"
"                                                       \n"
"                                                       \n"
"   _iso.AddEntity(_sprite[0], 0, 0, 0);                \n"
"                                                       \n"
"   return true;                                        \n"
"}                                                      \n";

bool fail = false;
int number = 0;
int number2 = 0;
COutStream out;
CScriptArray* GlobalCharArray = 0;

void print(const string &)
{
}

int getInt()
{
	return 42;
}

void ArrayToHexStr(asIScriptGeneric *gen)
{
}

asIScriptEngine *ConfigureEngine(int version)
{
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptArray(engine, true);
	RegisterScriptString(engine);

	// Register a property with the built-in array type
	GlobalCharArray = (CScriptArray*)engine->CreateScriptObject(engine->GetTypeIdByDecl("uint8[]"));
	int r = engine->RegisterGlobalProperty("uint8[] GlobalCharArray", GlobalCharArray); assert( r >= 0 );

	// Register function that use the built-in array type
	r = engine->RegisterGlobalFunction("string@ ARRAYTOHEX(uint8[] &in)", asFUNCTION(ArrayToHexStr), asCALL_GENERIC); assert( r >= 0 );


	if( version == 1 )
	{
		// The order of the properties shouldn't matter
		engine->RegisterGlobalProperty("int number", &number);
		engine->RegisterGlobalProperty("int number2", &number2);
	}
	else
	{
		// The order of the properties shouldn't matter
		engine->RegisterGlobalProperty("int number2", &number2);
		engine->RegisterGlobalProperty("int number", &number);
	}
	engine->RegisterObjectType("OBJ", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);

	return engine;
}

void TestScripts(asIScriptEngine *engine)
{
	int r;

	// Bind the imported functions
	asIScriptModule *mod = engine->GetModule(0);
	r = mod->BindAllImportedFunctions(); assert( r >= 0 );

	// Verify if handles are properly resolved
	asIScriptFunction *func = mod->GetFunctionByDecl("void TestHandle(string @)");
	if( func == 0 ) 
	{
		printf("%s: Failed to identify function with handle\n", TESTNAME);
		TEST_FAILED;
	}

	ExecuteString(engine, "main()", mod);

	if( number != 1234567890 )
	{
		printf("%s: Failed to set the number as expected\n", TESTNAME);
		TEST_FAILED;
	}

	// Call an interface method on a class that implements the interface
	int typeId = engine->GetModule(0)->GetTypeIdByDecl("MyClass");
	asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(typeId);

	int intfTypeId = engine->GetModule(0)->GetTypeIdByDecl("MyIntf");
	asIObjectType *type = engine->GetObjectTypeById(intfTypeId);
	func = type->GetMethodByDecl("void test()");
	asIScriptContext *ctx = engine->CreateContext();
	r = ctx->Prepare(func);
	if( r < 0 ) TEST_FAILED;
	ctx->SetObject(obj);
	ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	if( ctx ) ctx->Release();
	if( obj ) obj->Release();

	if( number != 1241 )
	{
		printf("%s: Interface method failed\n", TESTNAME);
		TEST_FAILED;
	}
}

void ConstructFloatArray(vector<float> *p)
{
	new(p) vector<float>;
}

void ConstructFloatArray(int s, vector<float> *p)
{
	new(p) vector<float>(s);
}

void DestructFloatArray(vector<float> *p)
{
	p->~vector<float>();
}

void IsoMapFactory(asIScriptGeneric *gen)
{
	*(int**)gen->GetAddressOfReturnLocation() = new int(1);
}

void IsoSpriteFactory(asIScriptGeneric *gen)
{
	*(int**)gen->GetAddressOfReturnLocation() = new int(1);
}

void DummyAddref(asIScriptGeneric *gen)
{
	int *object = (int*)gen->GetObject();
	(*object)++;
}

void DummyRelease(asIScriptGeneric *gen)
{
	int *object = (int*)gen->GetObject();
	(*object)--;
	if( *object == 0 )
		delete object;
}

void Dummy(asIScriptGeneric *gen)
{
}

static string _out;
void output(asIScriptGeneric *gen)
{
	string *str = (string*)gen->GetArgAddress(0);
	_out += *str;
}

bool Test2();

class Tmpl
{
public:
	Tmpl() {refCount = 1;}
	void AddRef() {refCount++;}
	void Release() {if( --refCount == 0 ) delete this;}
	static Tmpl *TmplFactory(asIObjectType*) {return new Tmpl;}
	static bool TmplCallback(asIObjectType *ot, bool &dontGC) {return false;}
	int refCount;
};

bool TestAndrewPrice();

bool Test()
{
	int r;
	COutStream out;
	asIScriptEngine* engine;
	asIScriptModule* mod;
	
	// Test multiple modules with shared enums and shared classes
	// http://www.gamedev.net/topic/632922-huge-problems-with-precompilde-byte-code/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"shared enum ResourceType {} \n"
			"shared class Resource \n"
			"{ \n"
			"	void getType(ResourceType) {} \n"
			"} \n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__"shared");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test problem reported by Andre Santee
	// http://www.gamedev.net/topic/635623-assertion-failed-while-using-function-handles/
	if( !strstr(asGetLibraryOptions(), "AS_NO_MEMBER_INIT") )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Foo \n"
			"{ \n"
			"    int a = 42; \n"
			"} \n"
			"class Bar \n"
			"{ \n"
			"    float b = 3.14f; \n"
			"} \n"
			"funcdef void TEST_FUNC_HANDLE(Foo, Bar); \n"
			"void testFunction(TEST_FUNC_HANDLE@ func) \n"
			"{ \n"
			"    func(Foo(), Bar()); \n"
			"} \n"
			"void callback(Foo f, Bar b) \n"
			"{ \n"
			"  assert( f.a == 42 && b.b == 3.14f ); \n"
			"  called = true; \n"
			"} \n"
			"bool called = false;\n");

		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__"shared");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "testFunction(callback); assert( called );", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}


	Test2();
	TestAndrewPrice();


	// Test saving/loading with array of function pointers
	// http://www.gamedev.net/topic/627737-bytecode-loading-error/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"funcdef void F(); \n"
			"array<F@> arr = { f }; \n"
			"void f() {} \n");

		r = mod->Build(); 
		if( r < 0 )
			TEST_FAILED;
		
		CBytecodeStream stream(__FILE__"1");
		
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream); 
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test shared interface with function pointers
	// http://www.gamedev.net/topic/639243-funcdef-inside-shared-interface-interface-already-implement-warning/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"funcdef void fdef(); \n"
			"shared interface iface \n"
			"{ \n"
			"	fdef@ dummy(); \n"
			"} \n");

		r = mod->Build(); 
		if( r < 0 )
			TEST_FAILED;
		
		CBytecodeStream stream(__FILE__"1");
		
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		
		engine->Release();

		// Load the bytecode in two different modules
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		stream.Restart();
		mod = engine->GetModule("A", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		stream.Restart();
		mod = engine->GetModule("B", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();

		// Load the bytecode twice, replacing the module
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

		stream.Restart();
		mod = engine->GetModule("A", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		stream.Restart();
		mod = engine->GetModule("A", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	if( !strstr(asGetLibraryOptions(), "AS_NO_MEMBER_INIT") )
	{
		engine = ConfigureEngine(0);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":1", script1, strlen(script1), 0);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		// Validate the number of global functions
		if( mod->GetFunctionCount() != 6 )
			TEST_FAILED;

		mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":2", script2, strlen(script2), 0);
		mod->Build();

		TestScripts(engine);
		asUINT currentSize, totalDestroyed, totalDetected;
		engine->GetGCStatistics(&currentSize, &totalDestroyed, &totalDetected);

		// Save the compiled byte code
		CBytecodeStream stream(__FILE__"1");
		CBytecodeStream stream2(__FILE__"2");
		mod = engine->GetModule(0);
		mod->SaveByteCode(&stream);
		mod->SaveByteCode(&stream2, true);

#ifndef STREAM_TO_FILE
		if( stream.buffer.size() != 2112 )
			printf("The saved byte code is not of the expected size. It is %d bytes\n", stream.buffer.size());
		asUINT zeroes = stream.CountZeroes();
		if( zeroes != 604 )
		{
			printf("The saved byte code contains a different amount of zeroes than the expected. Counted %d\n", zeroes);
			// Mac OS X PPC has more zeroes, probably due to the bool type being 4 bytes
		}
		asDWORD crc32 = ComputeCRC32(&stream.buffer[0], asUINT(stream.buffer.size()));
		if( crc32 != 0x6F8E198B )
			printf("The saved byte code has different checksum than the expected. Got 0x%X\n", crc32);

		// Without debug info
		if( stream2.buffer.size() != 1771 )
			printf("The saved byte code without debug info is not of the expected size. It is %d bytes\n", stream2.buffer.size());
		zeroes = stream2.CountZeroes();
		if( zeroes != 500 )
			printf("The saved byte code without debug info contains a different amount of zeroes than the expected. Counted %d\n", zeroes);
#endif
		// Test loading without releasing the engine first
		if( mod->LoadByteCode(&stream) != 0 )
			TEST_FAILED;

		if( mod->GetFunctionCount() != 6 )
			TEST_FAILED;

		if( string(mod->GetFunctionByIndex(0)->GetScriptSectionName()) != ":1" )
			TEST_FAILED;

		mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":2", script2, strlen(script2), 0);
		mod->Build();

		TestScripts(engine);

		// Test loading for a new engine
		GlobalCharArray->Release();
		GlobalCharArray = 0;

		engine->Release();
		engine = ConfigureEngine(1);

		stream2.Restart();
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream2);

		if( mod->GetFunctionCount() != 6 )
			TEST_FAILED;

		mod = engine->GetModule("DynamicModule", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(":2", script2, strlen(script2), 0);
		mod->Build();

		TestScripts(engine);
		asUINT currentSize2, totalDestroyed2, totalDetected2;
		engine->GetGCStatistics(&currentSize2, &totalDestroyed2, &totalDetected2);
		assert( currentSize == currentSize2 &&
				totalDestroyed == totalDestroyed2 &&
				totalDetected == totalDetected2 );

		GlobalCharArray->Release();
		GlobalCharArray = 0;
		engine->Release();

		//---------------------------------------
		// A tiny file for comparison
#ifndef STREAM_TO_FILE
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", "void f() {}");
		mod->Build();
		CBytecodeStream streamTiny(__FILE__"tiny");
		mod->SaveByteCode(&streamTiny, true);
		engine->Release();

		asBYTE expected[] = {0x01,0x00,0x00,0x00,0x00,0x00,0x01,0x66,0x6E,0x01,0x66,0x40,0x4E,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x3F,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x72,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		bool match = true;
		for( asUINT n = 0; n < streamTiny.buffer.size(); n++ )
			if( streamTiny.buffer[n] != expected[n] )
			{
				match = false;
				break;
			}
		if( !match )
		{
			printf("Tiny module gave a different result than expected:\n");
			printf("got     : ");
			for( asUINT n = 0; n < streamTiny.buffer.size(); n++ )
				printf("%0.2X", streamTiny.buffer[n]);
			printf("\n");
			printf("expected: ");
			for( asUINT m = 0; m < sizeof(expected); m++ )
				printf("%0.2X", expected[m]);
			printf("\n");
		}
#endif
	}

	// Test saving/loading global variable of registered value type
	// http://www.gamedev.net/topic/638529-wrong-function-called-on-bytecode-restoration/
	{
		struct A
		{
			static void Construct1(int *a) { *a = 1; }
			static void Construct2(int *a) { *a = 2; }
		};

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->RegisterObjectType("A", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
		engine->RegisterObjectBehaviour("A", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(A::Construct1), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectProperty("A", "int val", 0);

		engine->RegisterObjectType("B", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
		engine->RegisterObjectBehaviour("B", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(A::Construct2), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectProperty("B", "int val", 0);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("A", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "A a; B b;");
		r = mod->Build();
		if( r != 0 )
			TEST_FAILED;

		CBytecodeStream stream2(__FILE__"2");
		mod->SaveByteCode(&stream2);

		engine->Release();

		// Register the types in a different order this time
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->RegisterObjectType("B", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
		engine->RegisterObjectBehaviour("B", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(A::Construct2), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectProperty("B", "int val", 0);

		engine->RegisterObjectType("A", sizeof(int), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
		engine->RegisterObjectBehaviour("A", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(A::Construct1), asCALL_CDECL_OBJLAST);
		engine->RegisterObjectProperty("A", "int val", 0);

		mod = engine->GetModule("A", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream2);
		if( r != 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( a.val == 1 ); \n"
								  "assert( b.val == 2 ); \n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	//-----------------------------------------
	// A different case
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script3", script3, strlen(script3));
		mod->Build();
		CBytecodeStream stream2(__FILE__"2");
		mod->SaveByteCode(&stream2);

		engine->Release();
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream2);
		ExecuteString(engine, "Test(3)", mod);

		engine->Release();
	}

	//-----------------------------------
	// save/load with overloaded array types should work as well
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterScriptArray(engine, true);
		int r = engine->RegisterObjectType("float[]", sizeof(vector<float>), asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert(r >= 0);
#ifndef AS_MAX_PORTABILITY
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(ConstructFloatArray, (vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ConstructFloatArray, (int, vector<float> *), void), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructFloatArray), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "float[] &opAssign(float[]&in)", asMETHODPR(vector<float>, operator=, (const std::vector<float> &), vector<float>&), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "float &opIndex(int)", asMETHODPR(vector<float>, operator[], (vector<float>::size_type), float &), asCALL_THISCALL); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "int length()", asMETHOD(vector<float>, size), asCALL_THISCALL); assert(r >= 0);
#else
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST_PR(ConstructFloatArray, (vector<float> *), void), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_CONSTRUCT, "void f(int)", WRAP_OBJ_LAST_PR(ConstructFloatArray, (int, vector<float> *), void), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectBehaviour("float[]", asBEHAVE_DESTRUCT, "void f()", WRAP_OBJ_LAST(DestructFloatArray), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "float[] &opAssign(float[]&in)", WRAP_MFN_PR(vector<float>, operator=, (const std::vector<float> &), vector<float>&), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "float &opIndex(int)", WRAP_MFN_PR(vector<float>, operator[], (vector<float>::size_type), float &), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterObjectMethod("float[]", "int length()", WRAP_MFN(vector<float>, size), asCALL_GENERIC); assert(r >= 0);
#endif

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script3", script3, strlen(script3));
		mod->Build();
		
		CBytecodeStream stream3(__FILE__"3");
		mod->SaveByteCode(&stream3);
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream3);
		ExecuteString(engine, "Test(3)", mod);
		
		engine->Release();
	}

	//------------------------------------
	// Test problem detected by TheAtom
	// http://www.gamedev.net/topic/623170-crash-on-bytecode-loading/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		COutStream out;
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		asIScriptModule *mod = engine->GetModule("0", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("0",
			"shared class T\n"
			"{\n"
			"        void f() { }\n"
			"};\n"
			"shared class T2 : T\n"
			"{\n"
			"};\n"
			"class T3 : T\n"
			"{\n"
			"        void f() { T::f(); }\n"
			"};\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__"0");
		mod->SaveByteCode(&stream);

		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		stream.Restart();

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	//---------------------------------
	// Must be possible to load scripts with classes declared out of order
	// Built-in array types must be able to be declared even though the complete script structure hasn't been loaded yet
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterScriptString(engine);
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script4, strlen(script4));
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		else
		{
			// Test the script with compiled byte code
			asIScriptContext *ctx = engine->CreateContext();
			r = ExecuteString(engine, "g_inGame.Initialize(0);", mod, ctx);
			if( r != asEXECUTION_FINISHED )
			{
				if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
				TEST_FAILED;
			}
			if( ctx ) ctx->Release();

			// Save the bytecode
			CBytecodeStream stream4(__FILE__"4");
			mod = engine->GetModule(0);
			mod->SaveByteCode(&stream4);
			engine->Release();

			// Now load the bytecode into a fresh engine and test the script again
			engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
			RegisterScriptArray(engine, true);
			RegisterScriptString(engine);
			mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
			mod->LoadByteCode(&stream4);
			r = ExecuteString(engine, "g_inGame.Initialize(0);", mod);
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
		}
		engine->Release();
	}
	//----------------
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);
		RegisterScriptString(engine);
		r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("IsoSprite", sizeof(int), asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_FACTORY, "IsoSprite@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoSprite", "IsoSprite &opAssign(const IsoSprite &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoSprite", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

		r = engine->RegisterObjectType("IsoMap", sizeof(int), asOBJ_REF); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_FACTORY, "IsoMap@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "IsoMap &opAssign(const IsoMap &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "bool AddEntity(const IsoSprite@+, int col, int row, int layer)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectMethod("IsoMap", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script5, strlen(script5));
		r = mod->Build();
		if( r < 0 ) 
			TEST_FAILED;
		else
		{
			// Test the script with compiled byte code
			asIScriptContext *ctx = engine->CreateContext();
			r = ExecuteString(engine, "Initialize();", mod, ctx);
			if( r != asEXECUTION_FINISHED )
			{
				if( r == asEXECUTION_EXCEPTION ) PrintException(ctx);
				TEST_FAILED;
			}
			if( ctx ) ctx->Release();

			// Save the bytecode
			CBytecodeStream stream(__FILE__"5");
			mod = engine->GetModule(0);
			mod->SaveByteCode(&stream);
			engine->Release();

			// Now load the bytecode into a fresh engine and test the script again
			engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
			engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
			RegisterScriptArray(engine, true);
			RegisterScriptString(engine);
			r = engine->RegisterGlobalFunction("void Assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

			r = engine->RegisterObjectType("IsoSprite", sizeof(int), asOBJ_REF); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_FACTORY, "IsoSprite@ f()", asFUNCTION(IsoSpriteFactory), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoSprite", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectMethod("IsoSprite", "IsoSprite &opAssign(const IsoSprite &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectMethod("IsoSprite", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

			r = engine->RegisterObjectType("IsoMap", sizeof(int), asOBJ_REF); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_FACTORY, "IsoMap@ f()", asFUNCTION(IsoMapFactory), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_ADDREF, "void f()", asFUNCTION(DummyAddref), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectBehaviour("IsoMap", asBEHAVE_RELEASE, "void f()", asFUNCTION(DummyRelease), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectMethod("IsoMap", "IsoMap &opAssign(const IsoMap &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectMethod("IsoMap", "bool AddEntity(const IsoSprite@+, int col, int row, int layer)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );
			r = engine->RegisterObjectMethod("IsoMap", "bool Load(const string &in)", asFUNCTION(Dummy), asCALL_GENERIC); assert( r >= 0 );

			mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
			mod->LoadByteCode(&stream);
			r = ExecuteString(engine, "Initialize();", mod);
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
		}
		engine->Release();
	}

	//------------------------------
	// Test to make sure the script constants are stored correctly
	{
		const char *script = "void main()                 \n"
		"{                                                \n"
		"	int i = 123;                                  \n"
		"                                                 \n"
		"   output( ' i = (' + i + ')' + 'aaa' + 'bbb' ); \n"
		"}                                                \n";
		
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		r = engine->RegisterGlobalFunction("void output(const string &in)", asFUNCTION(output), asCALL_GENERIC); assert( r >= 0 );

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		ExecuteString(engine, "main()", mod);
		if( _out != " i = (123)aaabbb" )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		r = engine->RegisterGlobalFunction("void output(const string &in)", asFUNCTION(output), asCALL_GENERIC); assert( r >= 0 );
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		
		mod->LoadByteCode(&stream);

		_out = "";
		ExecuteString(engine, "main()", mod);
		if( _out != " i = (123)aaabbb" )
			TEST_FAILED;

		engine->Release();
	}

	//-------------------------------
	// Test that registered template classes are stored correctly
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"void main() \n"
			"{ \n"
			"	array< int > intArray = {0,1,2}; \n"
			"	uint tmp = intArray.length(); \n"
			"   assert( tmp == 3 ); \n"
			"}; \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->LoadByteCode(&stream);

		if( ExecuteString(engine, "main()", mod) != asEXECUTION_FINISHED )
			TEST_FAILED;
		
		engine->Release();
	}

	// Test loading script with out of order template declarations 
	{
		const char *script = 
			"class HogeManager \n"
			"{ \n"
			"  array< Hoge >@ hogeArray; \n"
			"} \n"
			"class Hoge {}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test loading script with out of order template declarations 
	{
		const char *script = 
			"class HogeManager \n"
			"{ \n"
			"  HogeManager() \n"
			"  { \n"
			"    array< Hoge >@ hogeArray; \n"
			"  } \n"
			"} \n"
			"class Hoge {}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test loading byte code that uses the enum types
	{
		const char *script = 
			"array< ColorKind > COLOR_KIND_TABLE = { ColorKind_Red }; \n"
			"enum ColorKind { ColorKind_Red }; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test loading and executing bytecode
	{
		const char *script = 
			"interface IObj {}; \n"
		    "class Hoge : IObj {}; \n"
		    "void main(int a = 0) \n"
		    "{ \n"
		    "    Hoge h; \n"
		    "    IObj@ objHandle = h; \n"
		    "    Hoge@ hogeHandle = cast< Hoge@ >( objHandle ); \n"
		    "}; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test that property offsets are properly mapped
	{
		asQWORD test;

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterObjectType("test", 8, asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectProperty("test", "int a", 0);
		engine->RegisterObjectProperty("test", "int b", 4);

		engine->RegisterGlobalProperty("test t", &test);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, "void func() { t.a = 1; t.b = 2; }");
		mod->Build();

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( *(int*)(&test) != 1 || *((int*)(&test)+1) != 2 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterObjectType("test", 8, asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectProperty("test", "int a", 4); // Switch order of the properties
		engine->RegisterObjectProperty("test", "int b", 0);

		engine->RegisterGlobalProperty("test t", &test);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( *(int*)(&test) != 2 || *((int*)(&test)+1) != 1 )
			TEST_FAILED;

		engine->Release();
	}

	// Test that value types are adjusted for different sizes
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->RegisterObjectType("test", 4, asOBJ_VALUE | asOBJ_POD);
		engine->RegisterObjectProperty("test", "int16 a", 0);
		engine->RegisterObjectProperty("test", "int16 b", 2);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, 
			"void func() { int a = 1; test b; int c = 2; b.a = a; b.b = c; check(b); } \n"
			"void check(test t) { assert( t.a == 1 ); \n assert( t.b == 2 ); \n } \n");
		mod->Build();

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		engine->RegisterObjectType("test", 8, asOBJ_VALUE | asOBJ_POD); // Different size
		engine->RegisterObjectProperty("test", "int16 a", 4); // Switch order of the properties
		engine->RegisterObjectProperty("test", "int16 b", 0);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "func()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test loading and executing bytecode
	{
		const char *script = 
			"interface ITest\n"
			"{\n"
			"}\n"
			"class Test : ITest\n"
			"{\n"
			"	ITest@[] arr;\n"
			"	void Set(ITest@ e)\n"
			"	{\n"
			"		arr.resize(1);\n"
			"		@arr[0]=e;\n"
			"	}\n"
			"}\n"
			"void main()\n"
			"{\n"
			"	Test@ t=Test();\n"
			"	t.Set(t);\n"
			"}\n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__);
		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test 
	{
		CBytecodeStream stream(__FILE__"1");

		const char *script = 
			"interface ITest1 { } \n"
			"interface ITest2 { } \n"
			" \n"
			"CTest@[] Array1; \n"
			" \n"
			"class CTest : ITest1 \n"
			"{ \n"
			"	CTest() \n"
			"	{ \n"
			"		Index=0; \n"
			"		@Field=null; \n"
			"	} \n"
			" \n"
			"	int Index; \n"
			"	ITest2@ Field; \n"
			"} \n"
			" \n"
			"int GetTheIndex() \n"
			"{ \n"
			"  return Array1[0].Index; \n"
			"} \n"
			" \n"
			"void Test() \n"
			"{ \n"
			"  Array1.resize(1); \n"
			"  CTest test(); \n"
			"  @Array1[0] = test; \n"
			"  GetTheIndex(); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();

		r = ExecuteString(engine, "Test()", mod);

		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterScriptArray(engine, true);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "Test()", mod);

		engine->Release();
	}

	// Test two modules with same interface
	{
		CBytecodeStream stream(__FILE__"1");

		const char *script = 
			"interface ITest \n"
			"{ \n"
			"  ITest@ test(); \n"
			"} \n"
			"class CTest : ITest \n"
			"{ \n"
			"  ITest@ test() \n"
			"  { \n"
			"    return this; \n"
			"  } \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();

		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		stream.Restart();
		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test loading bytecode, where the code uses a template instance that the template callback doesn't allow
	// The loading of the bytecode must fail graciously in this case, and display intelligent error message to 
	// allow the script writer to find the error in the original code.
	{
		CBytecodeStream stream(__FILE__"1");

		const char *script = 
			"tmpl<int> t; \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		r = engine->RegisterObjectType("tmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_FACTORY, "tmpl<T>@ f(int&in)", asFUNCTIONPR(Tmpl::TmplFactory, (asIObjectType*), Tmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Tmpl,AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Tmpl,Release), asCALL_THISCALL); assert( r >= 0 );
#else
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_FACTORY, "tmpl<T>@ f(int&in)", WRAP_FN_PR(Tmpl::TmplFactory, (asIObjectType*), Tmpl*), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_ADDREF, "void f()", WRAP_MFN(Tmpl,AddRef), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_RELEASE, "void f()", WRAP_MFN(Tmpl,Release), asCALL_GENERIC); assert( r >= 0 );
#endif
		//r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in)", asFUNCTION(Tmpl::TmplCallback), asCALL_CDECL); assert( r >= 0 );

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod->SaveByteCode(&stream);
		engine->Release();

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		r = engine->RegisterObjectType("tmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_FACTORY, "tmpl<T>@ f(int&in)", asFUNCTIONPR(Tmpl::TmplFactory, (asIObjectType*), Tmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(Tmpl,AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(Tmpl,Release), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", asFUNCTION(Tmpl::TmplCallback), asCALL_CDECL); assert( r >= 0 );
#else
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_FACTORY, "tmpl<T>@ f(int&in)", WRAP_FN_PR(Tmpl::TmplFactory, (asIObjectType*), Tmpl*), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_ADDREF, "void f()", WRAP_MFN(Tmpl,AddRef), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_RELEASE, "void f()", WRAP_MFN(Tmpl,Release), asCALL_GENERIC); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("tmpl<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in, bool&out)", WRAP_FN(Tmpl::TmplCallback), asCALL_GENERIC); assert( r >= 0 );
#endif

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		bout.buffer = "";
		r = mod->LoadByteCode(&stream);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != " (0, 0) : Error   : Attempting to instanciate invalid template type 'tmpl<int>'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test problem reported by Philip Bennefall
	{
		const char *script = 
			"class dummy\n"
			"{\n"
			"  bool set_callback(menu_callback@ callback, string user_data)\n"
			"  {\n"
			"    @callback_handle=@callback;\n"
			"    callback_data=user_data;\n"
			"    return true;\n"
			"  }\n"
			"  void do_something()\n"
			"  {\n"
			"    if(@callback_handle!=null)\n"
			"    {\n"
			"      int callback_result=callback_handle(this, callback_data);\n"
			"    }\n"
			"  }\n"
			"  menu_callback@ callback_handle;\n"
			"  string callback_data;\n"
			"}\n"
			"funcdef int menu_callback(dummy@, string);\n"
			"void main()\n"
			"{\n"
			"}\n";

		CBytecodeStream stream(__FILE__"1");

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);

		mod = engine->GetModule("1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection(0, script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod->SaveByteCode(&stream);

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	// Test problem reported by Markus Larsson from Skygoblin
	{
		const char *script1 = 
			"void main() {"
			"  print(\"a\" + \"b\");"
			"}";

		const char *script2 =
			"void main() {"
			"  if(getInt()==1)"
			"    print(\"a\" + \"b\");"
			"}";

		int r;
		asIScriptContext* ctx;
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		RegisterStdString(engine);
#ifndef AS_MAX_PORTABILITY
		engine->RegisterGlobalFunction("void print(const string& in)", asFUNCTION(print), asCALL_CDECL);
		engine->RegisterGlobalFunction("int getInt()", asFUNCTION(getInt), asCALL_CDECL);
#else
		engine->RegisterGlobalFunction("void print(const string& in)", WRAP_FN(print), asCALL_GENERIC);
		engine->RegisterGlobalFunction("int getInt()", WRAP_FN(getInt), asCALL_GENERIC);
#endif
		
		ctx = engine->CreateContext();
		
		asIScriptModule* mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->AddScriptSection(":1", script1, strlen(script1), 0); assert (r >= 0);
		r = mod->Build(); 
		if( r < 0 )
			TEST_FAILED;
		
		CBytecodeStream stream(__FILE__"1");
		
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream); 
		if( r < 0 )
			TEST_FAILED;
		
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		CBytecodeStream stream2(__FILE__"2");
		r = mod->AddScriptSection(":1", script2, strlen(script2), 0); assert (r >= 0);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		
		r = mod->SaveByteCode(&stream2);
		if( r < 0 )
			TEST_FAILED;
		
		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream2);
		if( r < 0 )
			TEST_FAILED;
		
		ctx->Prepare(mod->GetFunctionByDecl("void main()"));
		r = ctx->Execute();		
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		ctx->Release();
		engine->Release();
	}

	// Test problem on 64bit
	// http://www.gamedev.net/topic/628452-linux-x86-64-not-loading-or-saving-bytecode-correctly/
	{
		asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert( bool )", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"enum TestEnum \n"
			"{ \n"
			"  TestEnum_A = 42 \n"
			"} \n"
			"class NonPrimitive \n"
			"{ \n"
			"  int val; \n"
			"} \n"
			"void Foo( int a, TestEnum e, NonPrimitive o ) \n"
			"{ \n"
			"  assert( a == 1 ); \n"
			"  assert( e == TestEnum_A ); \n"
			"  assert( o.val == 513 ); \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  NonPrimitive o; \n"
			"  o.val = 513; \n"
			"  Foo( 1, TestEnum_A, o ); \n"
			"} \n");

		int r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream(__FILE__"1");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		asIScriptModule *mod2 = engine->GetModule("mod2", asGM_ALWAYS_CREATE);
		r = mod2->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod2);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Success
	return fail;
}

bool Test2()
{
	int r;
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	const char *script = 
		"enum ENUM1{          \n"
		"_ENUM_1 = 1          \n"
		"}                    \n"
		"void main()          \n"
		"{                    \n"
		"int item = _ENUM_1;  \n"
		"}                    \n";

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	CBytecodeStream stream(__FILE__"6");
	mod = engine->GetModule(0);
	r = mod->SaveByteCode(&stream);
	if( r < 0 )
		TEST_FAILED;
	engine->Release();

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	r = mod->LoadByteCode(&stream);
	if( r < 0 )
		TEST_FAILED;

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->Release();

	return fail;
}



const char *APStringFactory(int length, const char *s)
{
	return s;
}

void APStringConstruct(const char **s)
{
	*s = 0;
}

bool TestAndrewPrice()
{
	COutStream out;
	CBufferedOutStream bout;

	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);
		engine->SetEngineProperty(asEP_COPY_SCRIPT_SECTIONS, true);
		RegisterScriptArray(engine, true);

		// This POD type doesn't have an opAssign, so the bytecode will have asBC_COPY 
		engine->RegisterObjectType("char_ptr", sizeof(char*), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
#ifndef AS_MAX_PORTABILITY
		engine->RegisterObjectBehaviour("char_ptr", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(APStringConstruct), asCALL_CDECL_OBJLAST);
		engine->RegisterStringFactory("char_ptr", asFUNCTION(APStringFactory), asCALL_CDECL);
#else
		engine->RegisterObjectBehaviour("char_ptr", asBEHAVE_CONSTRUCT, "void f()", WRAP_OBJ_LAST(APStringConstruct), asCALL_GENERIC);
		engine->RegisterStringFactory("char_ptr", WRAP_FN(APStringFactory), asCALL_GENERIC);
#endif
			
		asIScriptModule *mod = engine->GetModule("Test", asGM_ALWAYS_CREATE);
		char Data2[] = 
			"const char_ptr[] STORAGE_STRINGS = {'Storage[0]','Storage[1]'}; ";
		mod->AddScriptSection("Part2",Data2,(int)strlen(Data2));
		int r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		mod->BindAllImportedFunctions();

		CBytecodeStream stream(__FILE__"1");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		mod = engine->GetModule("Test2", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		CScriptArray *arr = reinterpret_cast<CScriptArray*>(mod->GetAddressOfGlobalVar(0));
		if( arr == 0 || arr->GetSize() != 2 || strcmp(*reinterpret_cast<const char**>(arr->At(1)), "Storage[1]") != 0 )
			TEST_FAILED;

		engine->Release();

		// Try loading the bytecode again, except this time without configuring the engine
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		
		mod = engine->GetModule("Test3", asGM_ALWAYS_CREATE);
		stream.Restart();
		bout.buffer = "";
		r = mod->LoadByteCode(&stream);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != " (0, 0) : Error   : Template type 'array' doesn't exist\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		RegisterScriptArray(engine, true);
		stream.Restart();
		bout.buffer = "";
		r = mod->LoadByteCode(&stream);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != " (0, 0) : Error   : Object type 'char_ptr' doesn't exist\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	return fail;
}

} // namespace

