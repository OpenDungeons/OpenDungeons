#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"


namespace Test_Addon_Dictionary
{

const char *script =
"void Test()                       \n"
"{                                 \n"
"  dictionary dict;                \n"
// Test integer with the dictionary
"  dict.set('a', 42);            \n"
"  assert(dict.exists('a'));     \n"
"  uint u = 0;                     \n"
"  dict.get('a', u);             \n"
"  assert(u == 42);                \n"
"  dict.delete('a');             \n"
"  assert(!dict.exists('a'));    \n"
// Test array by handle
"  array<string> a = {'t'};               \n"
"  dict.set('a', @a);            \n"
"  array<string> @b;                      \n"
"  dict.get('a', @b);            \n"
"  assert(b == a);             \n"
// Test string by value
"  dict.set('a', 't');             \n"
"  string c;                       \n"
"  dict.get('a', c);             \n"
"  assert(c == 't');             \n"
// Test int8 with the dictionary
"  int8 q = 41;                    \n"
"  dict.set('b', q);             \n"
"  dict.get('b', q);             \n"
"  assert(q == 41);                \n"
// Test float with the dictionary
"  float f = 300;                  \n"
"  dict.set('c', f);             \n"
"  dict.get('c', f);             \n"
"  assert(f == 300);               \n"
// Test automatic conversion between int and float in the dictionary
"  int i;                          \n"
"  dict.get('c', i);             \n"
"  assert(i == 300);               \n"
"  dict.get('b', f);             \n"
"  assert(f == 41);                \n"
// Test booleans with the variable type
"  bool bl;                        \n"
"  dict.set('true', true);       \n"
"  dict.set('false', false);     \n"
"  bl = false;                     \n"
"  dict.get('true', bl);         \n"
"  assert( bl == true );           \n"
"  dict.get('false', bl);        \n"
"  assert( bl == false );          \n"
// Test circular reference with itself
"  dict.set('self', @dict);      \n"
// Test the keys
"  array<string> @keys = dict.getKeys(); \n"
"  assert( keys.find('a') != -1 ); \n"
"  assert( keys.length == 6 ); \n"
"}                                 \n";

// Test circular reference including a script class and the dictionary
const char *script2 = 
"class C { dictionary dict; }                \n"
"void f() { C c; c.dict.set(\"self\", @c); } \n"; 

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
	CBufferedOutStream bout;
 	asIScriptEngine *engine = 0;

	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);
	RegisterScriptArray(engine, true);
	RegisterScriptDictionary(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	asIScriptContext *ctx = engine->CreateContext();
	r = ExecuteString(engine, "Test()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		TEST_FAILED;
	}
	ctx->Release();

	asUINT gcCurrentSize, gcTotalDestroyed, gcTotalDetected;
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
	engine->GarbageCollect();
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

	if( gcCurrentSize != 0 || gcTotalDestroyed != 2 || gcTotalDetected != 1 )
		TEST_FAILED;

	// Test circular references including a script class and the dictionary
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	r = ExecuteString(engine, "f()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);
	engine->GarbageCollect();
	engine->GetGCStatistics(&gcCurrentSize, &gcTotalDestroyed, &gcTotalDetected);

	if( gcCurrentSize != 0 || gcTotalDestroyed != 5 || gcTotalDetected != 3  )
		TEST_FAILED;

	// Test invalid ref cast together with the variable argument
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "dictionary d; d.set('hello', cast<int>(4));");
	if( r >= 0 ) 
		TEST_FAILED;
	if( bout.buffer != "ExecuteString (1, 35) : Error   : Illegal target type for reference cast\n" )
	{
		TEST_FAILED;
		printf("%s", bout.buffer.c_str());
	}

	engine->Release();

	//-------------------------
	// Test the generic interface as well
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterStdString(engine);
	RegisterScriptArray(engine, true);
	RegisterScriptDictionary_Generic(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	ctx = engine->CreateContext();
	r = ExecuteString(engine, "Test()", mod, ctx);
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		TEST_FAILED;
	}
	ctx->Release();

	engine->Release();

	//------------------------
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	
		const char *script =
			"class Test \n"
			"{ \n"
			"  Test() { dict.set('int', 1); dict.set('string', 'test'); dict.set('handle', @array<string>()); } \n"
			"  dictionary dict; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  Test test = Test(); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PrintException(ctx);
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, true);
		RegisterScriptDictionary(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	
		const char *script =
			"void main() \n"
			"{ \n"
			" dictionary test; \n"
			" test.set('test', 'something'); \n"
			" string output; \n"
			" test.get('test', output); \n"
			" assert(output == 'something'); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r != asEXECUTION_FINISHED )
		{
			if( r == asEXECUTION_EXCEPTION )
				PrintException(ctx);
			TEST_FAILED;
		}
		ctx->Release();

		engine->Release();
	}

	return fail;
}

} // namespace

