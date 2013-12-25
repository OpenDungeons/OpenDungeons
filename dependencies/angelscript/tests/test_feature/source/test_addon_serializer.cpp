#include "utils.h"
#include "../../../add_on/scriptarray/scriptarray.h"
#include "../../../add_on/serializer/serializer.h"

namespace Test_Addon_Serializer
{

struct CStringType : public CUserType
{
	void Store(CSerializedValue *val, void *ptr)
	{
		val->SetUserData(new std::string(*(std::string*)ptr));
	}
	void Restore(CSerializedValue *val, void *ptr)
	{
		std::string *buffer = (std::string*)val->GetUserData();
		*(std::string*)ptr = *buffer;
	}
	void CleanupUserData(CSerializedValue *val)
	{
		std::string *buffer = (std::string*)val->GetUserData();
		delete buffer;
	}
};

struct CArrayType : public CUserType
{
	void Store(CSerializedValue *val, void *ptr)
	{
		CScriptArray *arr = (CScriptArray*)ptr;

		for( unsigned int i = 0; i < arr->GetSize(); i++ )
			val->m_children.push_back(new CSerializedValue(val ,"", "", arr->At(i), arr->GetElementTypeId()));
	}
	void Restore(CSerializedValue *val, void *ptr)
	{
		CScriptArray *arr = (CScriptArray*)ptr;
		arr->Resize(val->m_children.size());

		for( size_t i = 0; i < val->m_children.size(); ++i )	
			val->m_children[i]->Restore(arr->At(i), arr->GetElementTypeId());
	}
};

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
 	asIScriptEngine *engine;
	asIScriptModule *mod;
	
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script = 
			"float f; \n"
			"string str; \n"
			"array<int> arr; \n"
			"class CTest \n"
			"{ \n"
			"  int a; \n"
			"  string str; \n"
			"} \n"
			"CTest @t; \n"
			"CTest a; \n"
			"CTest @b; \n"
			"CTest @t2 = @a; \n"
			"CTest @n = @a; \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "f = 3.14f; \n"
			                      "str = 'test'; \n"
								  "arr.resize(3); arr[0] = 1; arr[1] = 2; arr[2] = 3; \n"
								  "a.a = 42; \n"
								  "a.str = 'hello'; \n"
								  "@b = @a; \n"
								  "@t = CTest(); \n"
								  "t.a = 24; \n"
								  "t.str = 'olleh'; \n"
								  "@t2 = t; \n"
								  "@n = null; \n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		CSerializer modStore;
		modStore.AddUserType(new CStringType(), "string");
		modStore.AddUserType(new CArrayType(), "array");

		r = modStore.Store(mod);
		if( r < 0 )
			TEST_FAILED;

		engine->DiscardModule(0);

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = modStore.Restore(mod);
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "assert(f == 3.14f); \n"
		                          "assert(str == 'test'); \n"
								  "assert(arr.length() == 3 && arr[0] == 1 && arr[1] == 2 && arr[2] == 3); \n"
								  "assert(a.a == 42); \n"
								  "assert(a.str == 'hello'); \n"
								  "assert(b is a); \n"
								  "assert(t !is null); \n"
								  "assert(t.a == 24); \n"
								  "assert(t.str == 'olleh'); \n"
								  "assert(t is t2); \n"
								  "assert(n is null); \n", mod);

		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
	
		engine->Release();
	}

	// Make sure it is possible to restore objects, where the constructor itself is changing other objects
	// http://www.gamedev.net/topic/604890-dynamic-reloading-script/page__st__20
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		RegisterScriptArray(engine, false);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char* script =
			"array<TestScript@> arr; \n"
			"class TestScript \n"
			"{ \n"
			"         TestScript()   \n"
			"         { \n"
			"                  arr.insertLast( this ); \n"
			"         }       \n"
			"} \n"
			"void startGame()          \n"
			"{ \n"
			"         TestScript @t = TestScript(); \n"
			"}  \n";

		mod->AddScriptSection( 0, script );
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "startGame()", mod);

		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		CSerializer modStore;
		modStore.AddUserType(new CStringType(), "string");
		modStore.AddUserType(new CArrayType(), "array");

		r = modStore.Store(mod);
		if( r < 0 )
			TEST_FAILED;

		engine->DiscardModule(0);

		mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = modStore.Restore(mod);
		if( r < 0 )
			TEST_FAILED;

		engine->Release();
	}

	return fail;
}

} // namespace

