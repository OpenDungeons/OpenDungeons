#include "utils.h"

namespace TestModule
{

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	asIScriptContext *ctx;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	ctx = engine->CreateContext();
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	// Compile a single function
	asIScriptFunction *func = 0;
	r = mod->CompileFunction("My func", "void func() {}", 0, 0, &func);
	if( r < 0 )
		TEST_FAILED;

	// Execute the function
	r = ctx->Prepare(func);
	if( r < 0 )
		TEST_FAILED;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// The function's section name should be correct
	if( std::string(func->GetScriptSectionName()) != "My func" )
		TEST_FAILED;

	// We must release the function afterwards
	if( func )
	{
		func->Release();
		func = 0;
	}

	// It must not be allowed to include more than one function in the code
	bout.buffer = "";
	r = mod->CompileFunction("two funcs", "void func() {} void func2() {}", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	r = mod->CompileFunction("no code", "", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	r = mod->CompileFunction("var", "int a;", 0, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != "two funcs (0, 0) : Error   : The code must contain one and only one function\n"
					   "no code (0, 0) : Error   : The code must contain one and only one function\n"
					   "var (0, 0) : Error   : The code must contain one and only one function\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// Compiling without giving the function pointer shouldn't leak memory
	r = mod->CompileFunction(0, "void func() {}", 0, 0, 0);
	if( r < 0 )
		TEST_FAILED;

	// If the code is not provided, a proper error should be given
	r = mod->CompileFunction(0,0,0,0,0);
	if( r != asINVALID_ARG )
		TEST_FAILED;

	// Don't permit recursive calls, unless the function is added to the module scope
	// TODO: It may be possible to compile a recursive function even without adding
	//       it to the scope, but the application needs to explicitly allows it
	bout.buffer = "";
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, 0, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != " (1, 2) : Error   : No matching signatures to 'func()'\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	// It should be possible to add the compiled function to the scope of the module
	if( mod->GetFunctionCount() > 0 )
		TEST_FAILED;
	r = mod->CompileFunction(0, "void func() {}", 0, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetFunctionCount() != 1 )
		TEST_FAILED;

	// It should be possible to remove a function from the scope of the module
	r = mod->RemoveFunction(mod->GetFunctionByIndex(0));
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetFunctionCount() != 0 )
		TEST_FAILED;

	// Compiling recursive functions that are added to the module is OK
	r = mod->CompileFunction(0, "void func() {\n func(); \n}", -1, asCOMP_ADD_TO_MODULE, 0);
	if( r < 0 )
		TEST_FAILED;

	// It should be possible to remove global variables from the scope of the module
	mod->AddScriptSection(0, "int g_var; void func() { g_var = 1; }");
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 1 )
		TEST_FAILED;
	r = mod->RemoveGlobalVar(0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 0 )
		TEST_FAILED;
	r = ExecuteString(engine, "func()", mod);
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	// It should be possible to add new variables
	r = mod->CompileGlobalVar(0, "int g_var;", 0);
	if( r < 0 )
		TEST_FAILED;
	r = mod->CompileGlobalVar(0, "int g_var2 = g_var;", 0);
	if( r < 0 )
		TEST_FAILED;
	if( mod->GetGlobalVarCount() != 2 )
		TEST_FAILED;

	// Shouldn't be possible to add function with the same name as a global variable
	bout.buffer = "";
	r = mod->CompileFunction(0, "void g_var() {}", 0, asCOMP_ADD_TO_MODULE, 0);
	if( r >= 0 )
		TEST_FAILED;
	if( bout.buffer != " (1, 1) : Error   : Name conflict. 'g_var' is a global property.\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	if( ctx ) 
		ctx->Release();
	engine->Release();

	// TODO: Removing a function from the scope of the module shouldn't free it 
	//       immediately if it is still used by another function. This is working.
	//       I just need a formal test for regression testing.

	// TODO: Make sure cyclic references between functions are resolved so we don't get memory leaks
	//       This is working. I just need a formal test for regression testing.

	// TODO: Do not allow adding functions that already exist in the module

	// TODO: Maybe we can allow replacing an existing function

	// TODO: It should be possible to serialize these dynamic functions

	// TODO: The dynamic functions should also be JIT compiled

	// TODO: What should happen if a function in the module scope references another function that has 
	//       been removed from the scope but is still alive, and then the byte code for the module is saved?

	// Make sure a circular reference between global variable, class, and class method is properly released
	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	mod = engine->GetModule("script", asGM_ALWAYS_CREATE);
	const char *script = "obj o; class obj { void d() { o.val = 1; } int val; }";
	mod->AddScriptSection("script", script);
	bout.buffer = "";
	r = mod->Build();
	if( r != 0 )
		TEST_FAILED;
	if( bout.buffer != "" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}
	engine->Release();

	// Success
	return fail;
}

} // namespace

