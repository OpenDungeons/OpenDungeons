#include "utils.h"
#include "../../add_on/scriptmath/scriptmathcomplex.h"

using namespace std;

namespace TestDefaultArg
{

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	COutStream out;
	asIScriptModule *mod;
 	asIScriptEngine *engine;
	
	// Test to make sure the default arg evaluates to a type that matches the function parameter
	// Reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterStdString(engine);

		bout.buffer = "";

		mod = engine->GetModule("mod1", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"class ud_bool \n"
			"{ \n"
			"  bool value; \n"
			"  ud_bool() \n"
			"  { \n"
			"    value = false; \n"
			"  } \n"
			"  void  opAssign(bool data) \n"
			"  { \n"
			"    this.value = data; \n"
			"  } \n"
			"  bool opEquals(bool data) \n"
			"  { \n"
			"    if(this.value == data) \n"
			"      return true; \n"
			"    return false; \n"
			"  } \n"
			"} \n"
			"ud_bool kill_object; \n"
			"ud_bool@ kill=@kill_object; \n"
			"void main() \n"
			"{ \n"
			"  kill_all(); \n"
			"} \n"
			"void kill_all(bool only_you=kill) \n"
			"{ \n"
			"}\n");
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "test (21, 1) : Info    : Compiling void main()\n"
		                   "default arg (1, 1) : Error   : The type of the default argument expression doesn't match the function parameter type\n"
		                   "test (23, 3) : Error   : Failed while compiling default arg for parameter 0 in function 'void kill_all(bool arg0 = kill)'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test calling a function with default arg where the expression uses a global var
	// Reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", 
			"string g = 'global';\n"
			"void func(string a, string b = 'pre' + g + 'pos') \n"
			"{ \n"
			"  array<string> items; \n"
			"  assert(a == 'afirst'); \n"
			"  assert(b == 'preglobalpos'); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "array<string> arr = {'first', 'second'}; func('a'+arr[0]);\n", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Test calling a function with default argument
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterScriptString(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"void func(int b, const string &in a = 'default') \n"
			"{ \n"
			"  if( b == 0 ) \n"
			"    assert( a == 'default' ); \n"
			"  else  \n"
			"    assert( a == 'test' ); \n"
			"} \n" 
			"void main() \n"
			"{ \n"
			"  func(0); \n"
			"  func(0, 'default'); \n"
			"  func(1, 'test'); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Must be possible to register functions with default args as well
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		r = engine->RegisterGlobalFunction("void defarg(bool, int a = 34 + /* comments will be removed *""/ 45, int b = 23)", asFUNCTION(0), asCALL_GENERIC);
		if( r < 0 )
			TEST_FAILED;
		asIScriptFunction *func = engine->GetFunctionById(r);
		string decl = func->GetDeclaration();
		if( decl != "void defarg(bool, int arg1 = 34 + 45, int arg2 = 23)" )
		{
			printf("%s\n", decl.c_str());
			TEST_FAILED;
		}
		engine->Release();
	}

	// When default arg is used, all other args after that must have default args
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
		r = engine->RegisterGlobalFunction("void defarg(bool, int a = 34+45, int)", asFUNCTION(0), asCALL_GENERIC);
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != "System function (1, 1) : Error   : All subsequent parameters after the first default value must have default values in function 'void defarg(bool, int arg1 = 34 + 45, int)'\n"
			               " (0, 0) : Error   : Failed in call to function 'RegisterGlobalFunction' with 'void defarg(bool, int a = 34+45, int)' (Code: -10)\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		engine->Release();
	}

	// Shouldn't be possible to write default arg expressions that access local variables, globals are ok though
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"void func(int a = n) {} \n"
			"void main() \n"
			"{ \n"
			"  int n; \n"
			"  func(); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (2, 1) : Info    : Compiling void main()\n"
		                   "default arg (1, 1) : Error   : 'n' is not declared\n"
		                   "script (5, 3) : Error   : Failed while compiling default arg for parameter 0 in function 'void func(int arg0 = n)'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Default args in script class constructors
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"class T \n"
			"{ \n"
			"  T(int a, int b = 25) \n"
			"  { \n"
			"    assert(a == 10); \n"
			"    assert(b == 25); \n"
			"  } \n"
			"} \n" 
			"T g(10); \n"
			"void main() \n"
			"{ \n"
			"  T(10); \n"
			"  T l(10); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Default arg must not end up using variables that are used 
	// in previously compiled variables as temporaries
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
		RegisterStdString(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"void func(uint8 a, string b = 'b') \n"
			"{ \n"
			"  assert( a == 97 ); \n"
			"  assert( b == 'b' ); \n"
			"} \n" 
			"void main() \n"
			"{ \n"
			"  uint8 a; \n"
			"  func(a = 'a'[0]); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->Release();
	}

	// Shouldn't crash if attempting to call incorrect function
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script =
			"void myFunc( float f, int a=0, int b ) {} \n"
			"void main() \n"
			"{ \n"
			"  int n; \n"
			"  myFunc( 1.2, 6 ); \n"
			"} \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (1, 1) : Error   : All subsequent parameters after the first default value must have default values in function 'void myFunc(float, int arg1 = 0, int)'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test that default argument expression can access index in global array
	// Reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"int[] my_array(5);\n"
			"int index=3;\n"
			"void my_function(int arg1, int arg2=my_array[index])\n"
			"{\n"
			"  assert( arg2 == 42 ); \n"
			"}\n"
			"void main()\n"
			"{\n"
			"  my_array[index] = 42;\n"
			"  my_function(1);\n"
			"}\n";

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


	// Test invalid default argument expression
	// Reported by Philip Bennefall
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterScriptArray(engine, true);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"void my_function(int arg1, int arg2=my_array[i[])\n"
			"{\n"
			"}\n"
			"void main()\n"
			"{\n"
			"  my_function(1);\n"
			"}\n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r >= 0 )
			TEST_FAILED;

		if( bout.buffer != "script (4, 1) : Info    : Compiling void main()\n"
						   "default arg (1, 16) : Error   : Expected expression value\n"
						   "default arg (1, 17) : Error   : Expected ']'\n"
						   "script (6, 3) : Error   : Failed while compiling default arg for parameter 1 in function 'void my_function(int, int arg1 = my_array [ i [ ])'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->Release();
	}

	// Test use of class constructor in default argument
	// Reported by Thomas Grip
	{
		const char *script = 
			"void MyFunc(const complex &in avX = complex(1,1)){}";

		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		bout.buffer = "";
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);

		RegisterScriptMathComplex(engine);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

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

	// The test to make sure the saved bytecode keeps the default args is done in test_saveload.cpp
	// A test to make sure script class methods with default args work is done in test_saveload.cpp

	// TODO: The compilation of the default args must not add any LINE instructions in the byte code, because they wouldn't match the real script

	// Success
	return fail;
}



} // namespace

