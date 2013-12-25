#include "utils.h"
#include "scriptmath3d.h"

static const char * const TESTNAME = "TestVector3";

static const char *script =
"vector3 TestVector3()  \n"
"{                      \n"
"  vector3 v;           \n"
"  v.x=1;               \n"
"  v.y=2;               \n"
"  v.z=3;               \n"
"  return v;            \n"
"}                      \n"
"vector3 TestVector3Val(vector3 v)  \n"
"{                                  \n"
"  return v;                        \n"
"}                                  \n"
"void TestVector3Ref(vector3 &out v)\n"
"{                                  \n"
"  v.x=1;                           \n"
"  v.y=2;                           \n"
"  v.z=3;                           \n"
"}                                  \n";

bool TestVector3()
{
	bool fail = false;
	COutStream out;
	CBufferedOutStream bout;
	int r;
	asIScriptEngine *engine = 0;


	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptMath3D(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	Vector3 v;
	engine->RegisterGlobalProperty("vector3 v", &v);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection(TESTNAME, script);
	r = mod->Build();
	if( r < 0 )
	{
		printf("%s: Failed to build\n", TESTNAME);
		TEST_FAILED;
	}
	else
	{
		// Internal return
		r = ExecuteString(engine, "v = TestVector3();", mod);
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			TEST_FAILED;
		}
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			TEST_FAILED;
		}

		// Manual return
		v.x = 0; v.y = 0; v.z = 0;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByDecl("vector3 TestVector3()"));

		ctx->Execute();
		Vector3 *ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 1 || ret->y != 2 || ret->z != 3 )
		{
			printf("%s: Failed to assign correct Vector3\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Prepare(mod->GetFunctionByDecl("vector3 TestVector3Val(vector3)"));
		v.x = 3; v.y = 2; v.z = 1;
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		ret = (Vector3*)ctx->GetReturnObject();
		if( ret->x != 3 || ret->y != 2 || ret->z != 1 )
		{
			printf("%s: Failed to pass Vector3 by val\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Prepare(mod->GetFunctionByDecl("void TestVector3Ref(vector3 &out)"));
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		if( v.x != 1 || v.y != 2 || v.z != 3 )
		{
			printf("%s: Failed to pass Vector3 by ref\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Release();
	}

	// Assignment of temporary object
	r = ExecuteString(engine, "vector3 v; float x = (v = vector3(10.0f,7,8)).x; assert( x > 9.9999f && x < 10.0001f );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test some operator overloads
	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v*2).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (2*v).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v+v).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( v == vector3(1,0,0) );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "vector3 v(1,0,0); assert( (v *= 2).length() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test error message when constructor is not found
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "vector3 v = vector3(4,3,2,1);");
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	// TODO: the function signature for the constructors/factories should carry the name of the object instead of _beh_0_
	if( bout.buffer != "ExecuteString (1, 13) : Error   : No matching signatures to 'vector3(const int, const int, const int, const int)'\n"
					   "ExecuteString (1, 13) : Info    : Candidates are:\n"
					   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_()\n"
				   	   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_(const vector3&in)\n"
					   "ExecuteString (1, 13) : Info    : void vector3::_beh_0_(float, float arg1 = 0, float arg2 = 0)\n"
	                   "ExecuteString (1, 13) : Error   : Can't implicitly convert from 'const int' to 'vector3'.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	// Test allocation of value types on stack
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", "void func() { vector3 v; v.x = 1; assert( v.x == 1 ); assert( v.y == 0 ); }");
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		asIScriptFunction *func = mod->GetFunctionByName("func");
		if( func == 0 ) TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();

		ctx->Prepare(func);

		// During the execution of the function there should not be any
		// new allocations,  since the vector is allocated on the stack
        int allocs = GetNumAllocs(); UNUSED_VAR(allocs);

		r = ctx->Execute();
		if( r != asEXECUTION_FINISHED ) TEST_FAILED;

		// TODO: Why is it different on GNUC?
		/* TODO: Test this when turning on allocations on stack again
#ifdef __GNUC__
		if( (GetNumAllocs() - allocs) != 2 )
#else
        if( (GetNumAllocs() - allocs) != 0 )
#endif
		{
			printf("There were %d allocations during the execution\n", GetNumAllocs() - allocs);
			TEST_FAILED;
		}
		*/
		ctx->Release();
		engine->Release();
	}

	// Test passing value type by value
	{
		const char *script =
			"void testR(const vector3&in position) \n"
			"{ \n"
			"	assert( position.x == 1 ); \n"
			"	assert( position.y == 2 ); \n"
			"	assert( position.z == 3 ); \n"
			"} \n"
			"void testV(vector3 position) \n"
			"{ \n"
			"	assert( position.x == 1 ); \n"
			"	assert( position.y == 2 ); \n"
			"	assert( position.z == 3 ); \n"
			"} \n"
			"class T { \n"
			"  T() { pos = vector3(1,2,3); } \n"
			"  const vector3 &get_position() const { return pos; } \n"
			"  vector3 pos; \n"
			"} \n"
			"void start() \n"
			"{ \n"
			"	T t; \n"
			"	testR(t.position); \n"
			"	testV(t.position); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		// TODO: optimize: When calling the get_position method, a ref copy of the local object is done. 
		//                 This can be avoided since the local object cannot die before the returned reference is used.
		// TODO: optimize: When calling testV the vector3 is copied twice. Once to copy the returned reference to a local 
		//                 variable, and then to an object allocated on the heap for the function call. Once the value types
		//                 passed by value are no longer allocated on the heap this may be fixed on its own, but it needs to
		//                 be checked.
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		r = ExecuteString(engine, "start()", mod);
		if( r != asEXECUTION_FINISHED ) TEST_FAILED;

		engine->Release();
	}

	// Test use of the copy constructor
	{
		const char *script = 
			"class Camera \n"
			"{ \n"
			"  const vector3 &get_lookAt() const { return lookat; } \n"
			"  void set_lookAt(const vector3 &in v) { lookat = v; } \n"
			"  vector3 lookat; \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  Camera camera; \n"
			"  camera.lookat = vector3(1,2,3); \n"
			"  vector3 test = vector3(camera.lookAt); \n"
			"  vector3 test2(camera.lookAt); \n"
			"  assert( test.x == 1 ); \n"
			"  assert( test.y == 2 ); \n"
			"  assert( test.z == 3 ); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED ) TEST_FAILED;

		engine->Release();
	}

	// Test use of the value types in condition
	{
		const char *script = 
			"void main() \n"
			"{ \n"
			"  vector3 test = true ? vector3(1,2,3) : vector3(); \n"
			"  assert( test.x == 1 ); \n"
			"  assert( test.y == 2 ); \n"
			"  assert( test.z == 3 ); \n"
			"} \n";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		asIScriptContext *ctx = engine->CreateContext();
		r = ExecuteString(engine, "main()", mod, ctx);
		if( r == asEXECUTION_EXCEPTION )
			PrintException(ctx);
		if( r != asEXECUTION_FINISHED ) TEST_FAILED;
		ctx->Release();
		engine->Release();
	}

	//
	{
		const char *script = 
			"bool alert(string &in, string &in) {return true;} \n"
			"void main() \n"
			"{ \n"
			"  vector3 position(5, 0, 0); \n"
			"  alert('Length', '' + position.length()); \n"
			"  assert( position.length() == 5 ); \n"
			"} \n";


		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptMath3D(engine);
		RegisterStdString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		
		asIScriptModule *mod = engine->GetModule("mod", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 ) TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "vector3 pos(5, 0, 0); pos *= 5; assert( pos.length() == 25 )");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		r = ExecuteString(engine, "vector3 a(1); assert( a.x == 1 && a.y == 0 && a.z == 0 ); assert( a == vector3(1) );");
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	return fail;
}
