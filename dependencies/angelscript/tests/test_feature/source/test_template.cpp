#include "utils.h"

using namespace std;

namespace TestTemplate
{

class MyTmpl
{
public:
	MyTmpl(asIObjectType *t) 
	{
		refCount = 1;
		type = t;

		type->AddRef();
	}

	MyTmpl(asIObjectType *t, int len) 
	{
		refCount = 1;
		type = t;
		length = len;

		type->AddRef();
	}

	~MyTmpl()
	{
		if( type ) 
			type->Release();
	}

	void AddRef()
	{
		refCount++;
	}

	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}

	string GetNameOfType()
	{
		if( !type ) return "";

		string name = type->GetName();
		name += "<";
		name += type->GetEngine()->GetTypeDeclaration(type->GetSubTypeId());
		name += ">";

		return name;
	}

	MyTmpl &Assign(const MyTmpl &other)
	{
		return *this;
	}

	void SetVal(void *val)
	{
	}

	void *GetVal()
	{
		return 0;
	}

	asIObjectType *type;
	int refCount;

	int length;
};

MyTmpl *MyTmpl_factory(asIObjectType *type)
{
	return new MyTmpl(type);
}

MyTmpl *MyTmpl_factory(asIObjectType *type, int len)
{
	return new MyTmpl(type, len);
}

// A template specialization
class MyTmpl_float
{
public:
	MyTmpl_float()
	{
		refCount = 1;
	}
	~MyTmpl_float()
	{
	}
	void AddRef()
	{
		refCount++;
	}
	void Release()
	{
		if( --refCount == 0 )
			delete this;
	}
	string GetNameOfType()
	{
		return "MyTmpl<float>";
	}

	MyTmpl_float &Assign(const MyTmpl_float &other)
	{
		return *this;
	}

	void SetVal(float &val)
	{
	}

	float &GetVal()
	{
		return val;
	}

	int refCount;
	float val;
};

MyTmpl_float *MyTmpl_float_factory()
{
	return new MyTmpl_float();
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("Skipped due to max portability\n");
		return false;
	}

	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;

	// Test passing templates are arguments
	// http://www.gamedev.net/topic/639597-how-to-pass-arraystring-to-a-function/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterScriptArray(engine, false);
		RegisterStdString(engine);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"bool stringInList( array<string>& arg, string s ) \n"
			"{ \n"
			"	for( uint n = 0; n < arg.length(); n++ ) \n"
			"	{ \n"
			"		if( arg[n] == s ) \n"
			"			return true; \n"
			"	} \n"
			"	return false; \n"
			"}  \n"
			"array<string> gEngineRPMUnits = { 'stall-rpm','start-rpm','idle-rpm','redline-rpm','max-rpm' }; \n"
			"string VerifyEngineUnitSpec( string attribute, string unitSpec ) \n"
			"{ \n"
			"	// local \n"
			"	// string result; \n"
			"	// check for rpm \n"
			"	if( stringInList(gEngineRPMUnits,attribute) ) \n"
			"	{ \n"
			"		// unit has to be rpm, rps, rad/s \n"
			"		if( unitSpec!='revolution' ) \n"
			"			return 'error'; \n"
			"		else \n"
			"			return unitSpec; \n"
			"	} \n"
			"   return '';\n"
			"}\n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "assert( VerifyEngineUnitSpec('idle-rpm', 'revolution') == 'revolution' )", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// It should be possible to register a template type with multiple subtypes
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
		r = engine->RegisterObjectType("tmpl<class A, class B, class C>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Register a method for the template type
		bout.buffer = "";
		r = engine->RegisterObjectMethod("tmpl<A,B,C>", "C &func(const B &in, const A &in)", 0, asCALL_GENERIC);
		if( r < 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		// Get the template instance
		asIObjectType *type = engine->GetObjectTypeById(engine->GetTypeIdByDecl("tmpl<int, bool, float>"));
		if( type->GetSubTypeCount() != 3 )
			TEST_FAILED;
		if( type->GetSubTypeId(0) != asTYPEID_INT32 ||
			type->GetSubTypeId(1) != asTYPEID_BOOL ||
			type->GetSubTypeId(2) != asTYPEID_FLOAT )
			TEST_FAILED;

		asIScriptFunction *func = type->GetMethodByName("func");
		if( func->GetReturnTypeId() != asTYPEID_FLOAT )
			TEST_FAILED;
		if( func->GetParamTypeId(0) != asTYPEID_BOOL ||
			func->GetParamTypeId(1) != asTYPEID_INT32 )
			TEST_FAILED;

		engine->Release();
	}

	// Don't allow registering the same template name with different subtypes
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		bout.buffer = "";
		r = engine->RegisterObjectType("tmpl<class A, class B, class C>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		r = engine->RegisterObjectType("tmpl<class A>", 0, asOBJ_REF | asOBJ_TEMPLATE);
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Attempt compiling a script function that returns a multi subtype template
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = engine->RegisterObjectType("tmpl<class A>", 0, asOBJ_REF | asOBJ_TEMPLATE | asOBJ_NOCOUNT);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"tmpl<int,int> @func() { return a; } \n"
			"tmpl<int,int> @a; \n");
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "test (1, 1) : Error   : Template 'tmpl' expects 1 sub type(s)\n"
		                   "test (2, 1) : Error   : Template 'tmpl' expects 1 sub type(s)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Reported by zeta945@gmail.com
	// http://www.gamedev.net/topic/633458-cant-create-a-template-class-with-a-default-constructor/
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		
		r = engine->RegisterObjectType("myObj", 0, asOBJ_REF | asOBJ_NOCOUNT); assert(r >= 0);
		r = engine->RegisterObjectType("x<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_FACTORY, "x<T>@ f(int &in, int)", asFUNCTION(0), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_FACTORY, "x<T>@ f(int&in)", asFUNCTION(0), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(0), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("x<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(0), asCALL_THISCALL); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"void test() \n"
			"{ \n"
			"     x<myObj> xxx; \n"
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
	}
	

	// Reported by ThyReaper
	// array<const int> and array<const Obj@> doesn't give expected result
	{
		asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		RegisterScriptArray(engine, false);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"class Test {} \n"
			"void func() \n"
			"{ \n"
			"  array<const int> i(1); \n"
			"  array<const Test@> t(1); \n"
			"  i[0] = 1; \n" // should fail
			"  @t[0] = Test(); \n"
			"  t[0] = Test(); \n" // should fail
			"} \n");
		bout.buffer = "";
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (2, 1) : Info    : Compiling void func()\n"
					  	   "test (6, 8) : Error   : Reference is read-only\n"
						   "test (8, 8) : Error   : Reference is read-only\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}


	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterStdString(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE);
	if( r == asNOT_SUPPORTED )
	{
		printf("Skipping template test because it is not yet supported\n");
		engine->Release();
		return false;	
	}

	if( r < 0 )
		TEST_FAILED;

	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int&in)", asFUNCTIONPR(MyTmpl_factory, (asIObjectType*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

	// Must be possible to register properties for templates, but not of the template subtype
	r = engine->RegisterObjectProperty("MyTmpl<T>", "int length", asOFFSET(MyTmpl,length)); assert( r >= 0 );

	// Add method to return the type of the template instance as a string
	r = engine->RegisterObjectMethod("MyTmpl<T>", "string GetNameOfType()", asMETHOD(MyTmpl, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );

	// Add method that take and return the template type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "MyTmpl<T> &Assign(const MyTmpl<T> &in)", asMETHOD(MyTmpl, Assign), asCALL_THISCALL); assert( r >= 0 );

	// Add methods that take and return the template sub type
	r = engine->RegisterObjectMethod("MyTmpl<T>", "const T &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(const T& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	// Test that it is possible to instanciate the template type for different sub types
	r = ExecuteString(engine, "MyTmpl<int> i;    \n"
								 "MyTmpl<string> s; \n"
								 "assert( i.GetNameOfType() == 'MyTmpl<int>' ); \n"
								 "assert( s.GetNameOfType() == 'MyTmpl<string>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test that the assignment works
	r = ExecuteString(engine, "MyTmpl<int> i1, i2; \n"
		                         "i1.Assign(i2);      \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test that the template sub type works
	r = ExecuteString(engine, "MyTmpl<int> i; \n"
		                         "i.SetVal(0); \n"
								 "i.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// It should be possible to register specializations of the template type
	r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF); assert( r >= 0 );
	// The specialization's factory doesn't take the hidden asIObjectType parameter
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_FACTORY, "MyTmpl<float> @f()", asFUNCTION(MyTmpl_float_factory), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl_float, AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("MyTmpl<float>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl_float, Release), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "string GetNameOfType()", asMETHOD(MyTmpl_float, GetNameOfType), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "MyTmpl<float> &Assign(const MyTmpl<float> &in)", asMETHOD(MyTmpl_float, Assign), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "const float &GetVal() const", asMETHOD(MyTmpl, GetVal), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("MyTmpl<float>", "void SetVal(const float& in)", asMETHOD(MyTmpl, SetVal), asCALL_THISCALL); assert( r >= 0 );

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "assert( f.GetNameOfType() == 'MyTmpl<float>' ); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "MyTmpl<float> f1, f2; \n"
		                         "f1.Assign(f2);        \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "MyTmpl<float> f; \n"
		                         "f.SetVal(0); \n"
								 "f.GetVal(); \n");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	
	// TODO: Test behaviours that take and return the template sub type
	// TODO: Test behaviours that take and return the proper template instance type

	// TODO: Even though the template doesn't accept a value subtype, it must still be possible to register a template specialization for the subtype

	// TODO: Must be possible to allow use of initialization lists

	// TODO: Must allow the subtype to be another template type, e.g. array<array<int>>

	// TODO: Must allow multiple subtypes, e.g. map<string,int>



	engine->Release();

	// Test that a proper error occurs if the instance of a template causes invalid data types, e.g. int@
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTIONPR(MyTmpl_factory, (asIObjectType*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method makes it impossible to instanciate the template for primitive types
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T@)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		r = ExecuteString(engine, "MyTmpl<int> t;");
		if( r >= 0 )
		{
			TEST_FAILED;
		}

		if( bout.buffer != "ExecuteString (1, 8) : Error   : Can't instanciate template 'MyTmpl' with subtype 'int'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test that a template registered to take subtype by value cannot be instanciated for reference types
	{
		bout.buffer = "";
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		RegisterScriptString(engine);

		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in)", asFUNCTIONPR(MyTmpl_factory, (asIObjectType*), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		// This method makes it impossible to instanciate the template for reference types
		r = engine->RegisterObjectMethod("MyTmpl<T>", "void SetVal(T)", asFUNCTION(0), asCALL_GENERIC); assert( r >= 0 );
		
		r = ExecuteString(engine, "MyTmpl<string> t;");
		if( r >= 0 )
		{
			TEST_FAILED;
		}

		if( bout.buffer != "ExecuteString (1, 8) : Error   : Can't instanciate template 'MyTmpl' with subtype 'string'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}


	// The factory behaviour for a template class must have a hidden reference as first parameter (which receives the asIObjectType)
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f()", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int)", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		engine->Release();
	}

	// Must not allow registering properties with the template subtype 
	// TODO: Must be possible to use getters/setters to register properties of the template subtype
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF | asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectProperty("MyTmpl<T>", "T a", 0); 
		if( r != asINVALID_DECLARATION )
		{
			TEST_FAILED;
		}
		engine->Release();
	}

	// Must not be possible to register specialization before the template type
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		r = engine->RegisterObjectType("MyTmpl<float>", 0, asOBJ_REF);
		if( r != asINVALID_NAME )
		{
			TEST_FAILED;
		}
		engine->Release();
	}

	// Must properly handle templates without default constructor
	// http://www.gamedev.net/topic/617408-crash-when-aggregating-a-template-class-that-has-no-default-factory/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		CBufferedOutStream bout;
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = engine->RegisterObjectType("MyTmpl<class T>", 0, asOBJ_REF|asOBJ_TEMPLATE); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_FACTORY, "MyTmpl<T> @f(int &in, int)", asFUNCTIONPR(MyTmpl_factory, (asIObjectType*, int), MyTmpl*), asCALL_CDECL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(MyTmpl, AddRef), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("MyTmpl<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(MyTmpl, Release), asCALL_THISCALL); assert( r >= 0 );

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		// Class T should give error because there is no default constructor for t
		// Class S should work because it is calling the appropriate non-default constructor for s
		mod->AddScriptSection("mod", "class T { MyTmpl<int> t; } T t;\n"
			                         "class S { MyTmpl<int> s; S() { s = MyTmpl<int>(1); } } S s;");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "mod (1, 7) : Info    : Compiling T::T()\n"
		                   "mod (1, 23) : Error   : No default constructor for object of type 'MyTmpl'.\n"
						   "mod (2, 26) : Info    : Compiling S::S()\n"
		                   "mod (2, 34) : Error   : There is no copy operator for the type 'MyTmpl' available.\n"
						   "mod (2, 23) : Error   : No default constructor for object of type 'MyTmpl'.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		bout.buffer = "";
		r = ExecuteString(engine, "MyTmpl<int> t;");
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "ExecuteString (1, 13) : Error   : No default constructor for object of type 'MyTmpl'.\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Reported by slicer4ever
	// http://www.gamedev.net/topic/632288-registering-specialized-template-class/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		r = engine->RegisterObjectType("ConvexHull", 0, asOBJ_REF|asOBJ_NOCOUNT);
		r = engine->RegisterObjectType("LLObject<class T>", 0, asOBJ_REF|asOBJ_NOCOUNT|asOBJ_TEMPLATE);
		r = engine->RegisterObjectType("LLObject<ConvexHull@>", 0, asOBJ_REF|asOBJ_NOCOUNT);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

 	return fail;
}

} // namespace

