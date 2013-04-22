#include "utils.h"
#include "../../../add_on/debugger/debugger.h"

namespace Test_Addon_Debugger
{

class CMyDebugger : public CDebugger
{
public:
	CMyDebugger() : CDebugger() {}

	void Output(const std::string &str)
	{
		// Append output to local buffer instead of the screen
		output += str;
	}

	void TakeCommands(asIScriptContext *ctx)
	{
		// Simulate the user stepping through the execution
		InterpretCommand("s", ctx);
	}

	void LineCallback(asIScriptContext *ctx)
	{
		if( ctx->GetState() == asEXECUTION_ACTIVE )
		{
			// Call the command for listing local variables
			InterpretCommand("l v", ctx);

			// Invoke the original line callback
			CDebugger::LineCallback(ctx);
		}
	}

	std::string ToString(void *value, asUINT typeId, bool expandMembers, asIScriptEngine *engine)
	{
		// Interpret the string value
		if( typeId == engine->GetTypeIdByDecl("string") )
		{
			return "\"" + *reinterpret_cast<std::string*>(value) + "\"";
		}

		// Let debugger do the rest
		std::string str = CDebugger::ToString(value, typeId, expandMembers, engine);

		// This here is just so I can automate the test. All the addresses in this test
		// will be exactly the same, so we substitute it with XXXXXXXX to make sure the
		// test will be platform independent.
		if( str.length() > 0 && str[0] == '{' )
		{
			if( lastAddress == "" )
			{
				lastAddress = str;
				return "{XXXXXXXX}";
			}
			else if( str == lastAddress )
				return "{XXXXXXXX}";
			else
				return str;
		}

		return str;
	}

	std::string output;
	std::string lastAddress;
};

class CMyDebugger2 : public CDebugger
{
public:
	CMyDebugger2() : CDebugger() {}

	void Output(const std::string &str)
	{
		// Append output to local buffer instead of the screen
		output += str;
	}

	void TakeCommands(asIScriptContext *ctx)
	{
		// Suspend the context when we reach the break point
		ctx->Suspend();
	}

	std::string ToString(void *value, asUINT typeId, bool expandMembers, asIScriptEngine *engine)
	{
		// Let debugger do the rest
		std::string str = CDebugger::ToString(value, typeId, expandMembers, engine);

		// This here is just so I can automate the test. All the addresses in this test
		// will be exactly the same, so we substitute it with XXXXXXXX to make sure the
		// test will be platform independent.
		if( str.length() > 0 && str[0] == '{' )
		{
			if( lastAddress == "" )
			{
				lastAddress = str.substr(1,str.find("}", 0));
				return "{XXXXXXXX"+str.substr(lastAddress.length());
			}
			else if( str.substr(1,str.find("}", 0)) == lastAddress )
				return "{XXXXXXXX"+str.substr(lastAddress.length());
			else
				return str;
		}

		return str;
	}

	std::string lastAddress;
	std::string output;
};


bool Test()
{
	bool fail = false;
	int r;
	COutStream out;
 	asIScriptEngine *engine;
	asIScriptModule *mod;
	asIScriptContext *ctx;
	
	{
		CMyDebugger debug;
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		RegisterStdString(engine);

		const char *script = 
			"void func(int a, const int &in b, string c, const string &in d, type @e, type &f, type @&in g) \n"
			"{ \n"
			"} \n"
			"class type {} \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		ctx = engine->CreateContext();
		ctx->SetLineCallback(asMETHOD(CMyDebugger, LineCallback), &debug, asCALL_THISCALL);

		debug.InterpretCommand("s", ctx);

		r = ExecuteString(engine, "type t; func(1, 2, 'c', 'd', t, t, t)", mod, ctx);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		if( debug.output != "ExecuteString:1; void ExecuteString()\n"
							"ExecuteString:1; void ExecuteString()\n"
							"script:0; type@ type()\n"
							"script:0; type::type()\n"
							"type t = {XXXXXXXX}\n"
							"ExecuteString:1; void ExecuteString()\n"
							"int a = 1\n"
							"const int& b = 2\n"
							"string c = \"c\"\n"
							"const string& d = \"d\"\n"
							"type@ e = {XXXXXXXX}\n"
							"type& f = {XXXXXXXX}\n"
							"type@& g = {XXXXXXXX}\n"
							"script:3; void func(int, const int&in, string, const string&in, type@, type&inout, type@&in)\n"
							"int a = 1\n"
							"const int& b = 2\n"
							"string c = \"c\"\n"
							"const string& d = \"d\"\n"
							"type@ e = {XXXXXXXX}\n"
							"type& f = {XXXXXXXX}\n"
							"type@& g = {XXXXXXXX}\n"
							"script:3; void func(int, const int&in, string, const string&in, type@, type&inout, type@&in)\n"
							"type t = {XXXXXXXX}\n"
							"ExecuteString:2; void ExecuteString()\n" )
		{
			printf("%s", debug.output.c_str());
			TEST_FAILED;
		}

		ctx->Release();
		
		engine->Release();
	}

	// Test inspecting a script object
	// http://www.gamedev.net/topic/627854-debugger-crashes-when-evaluating-uninitialized-object-expression/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class CTest \n"
			"{ \n"
			"  CTest() { value = 42; } \n"
			"  int value; \n"
			"} \n"
			"void Func() \n"
			"{ \n"
			"  CTest t; \n"
			"  assert( t.value == 42 ); \n"
			"} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CMyDebugger2 debug;

		ctx = engine->CreateContext();
		ctx->SetLineCallback(asMETHOD(CMyDebugger, LineCallback), &debug, asCALL_THISCALL);

		// Set a break point on the line where the object will be created
		debug.InterpretCommand("b test:8", ctx);

		// Set a break point after the object has been created
		debug.InterpretCommand("b test:9", ctx);

		ctx->Prepare(mod->GetFunctionByName("Func"));

		// Before the function is actually executed the variables don't exist
		if( ctx->IsVarInScope(0) )
			TEST_FAILED;
		if( ctx->GetAddressOfVar(0) )
			TEST_FAILED;

		// It will break twice on line 8. Once when setting up the function stack frame, and then on the first line that is executed
		// TODO: The first SUSPEND in the bytecode should be optimized away as it is unnecessary
		for( int n = 0; n < 2; n++ )
		{
			r = ctx->Execute();
			if( r != asEXECUTION_SUSPENDED )
				TEST_FAILED;
			
			// Now we should be on the line where the object will be created created
			if( ctx->GetLineNumber() != 8 )
				TEST_FAILED;
			else
			{
				// The address should be null
				asIScriptObject *obj = (asIScriptObject*)ctx->GetAddressOfVar(0);
				if( obj != 0 )
					TEST_FAILED;

				debug.PrintValue("t", ctx);
			}
		}

		r = ctx->Execute();
		if( r != asEXECUTION_SUSPENDED )
			TEST_FAILED;

		// Now we should be on the line after the object has been created
		if( ctx->GetLineNumber() != 9 )
			TEST_FAILED;
		else
		{
			if( !ctx->IsVarInScope(0) )
				TEST_FAILED;

			asIScriptObject *obj = (asIScriptObject*)ctx->GetAddressOfVar(0);
			if( obj == 0 )
				TEST_FAILED;
			if( *(int*)obj->GetAddressOfProperty(0) != 42 )
				TEST_FAILED;

			debug.PrintValue("t", ctx);
		}

		if( debug.output != "Setting break point in file 'test' at line 8\n"
							"Setting break point in file 'test' at line 9\n"
							"Reached break point 0 in file 'test' at line 8\n"
							"test:8; void Func()\n"
							"Reached break point 0 in file 'test' at line 8\n"
							"test:8; void Func()\n"
							"Reached break point 1 in file 'test' at line 9\n"
							"test:9; void Func()\n"
							"{XXXXXXXX}\n"
							"  int value = 42\n" )
		{
			printf("%s", debug.output.c_str());
			TEST_FAILED;
		}

		ctx->Release();

		engine->Release();
	}

	return fail;
}

} // namespace

