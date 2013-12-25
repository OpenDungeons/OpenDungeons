#include <iostream>  // cout
#include <assert.h>  // assert()
#include <string.h>  // strstr()
#include <stdio.h>
#include <angelscript.h>
#include "../../../add_on/scriptbuilder/scriptbuilder.h"
#include "../../../add_on/scriptstdstring/scriptstdstring.h"
#include "../../../add_on/scriptarray/scriptarray.h"
#include "../../../add_on/scriptfile/scriptfile.h"
#include "../../../add_on/scripthelper/scripthelper.h"
#include "../../../add_on/debugger/debugger.h"

using namespace std;

// Function prototypes
int ConfigureEngine(asIScriptEngine *engine);
int CompileScript(asIScriptEngine *engine, const char *scriptFile);
int ExecuteScript(asIScriptEngine *engine, const char *scriptFile, bool debug);

// For the scripts
void PrintString(const string &str);
CScriptArray *GetCommandLineArgs();

CScriptArray *g_commandLineArgs = 0;
int           g_argc = 0;
char        **g_argv = 0;

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

int main(int argc, char **argv)
{
	int r;

	// Validate the command line arguments
	bool argsValid = true;
	if( argc < 2 ) 
		argsValid = false;
	else if( argc == 2 && strcmp(argv[1], "-d") == 0 )
		argsValid = false;

	if( !argsValid )
	{
		cout << "Usage: " << endl;
		cout << "asrun [-d] <script file> [<args>]" << endl;
		cout << " -d             inform if the script should be runned with debug" << endl;
		cout << " <script file>  is the script file that should be runned" << endl;
		cout << " <args>         zero or more args for the script" << endl;
		return -1;
	}

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will send any compiler messages to the callback
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	r = ConfigureEngine(engine);
	if( r < 0 ) return -1;
	
	// Check if the script is to be debugged
	bool debug = false;
	if( strcmp(argv[1], "-d") == 0 )
		debug = true;

	// Store the command line arguments for the script
	int scriptArg = debug ? 2 : 1;
	g_argc = argc - (scriptArg + 1);
	g_argv = argv + (scriptArg + 1);

	// Compile the script code
	r = CompileScript(engine, argv[scriptArg]);
	if( r < 0 ) return -1;

	// Execute the script
	r = ExecuteScript(engine, argv[scriptArg], debug);
	
	// Release the engine
	if( g_commandLineArgs )
		g_commandLineArgs->Release();
	engine->Release();

	return r;
}

// This function will register the application interface
int ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	RegisterStdString(engine);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);

	r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("array<string> @getCommandLineArgs()", asFUNCTION(GetCommandLineArgs), asCALL_CDECL); assert( r >= 0 );

	// TODO: There should be an option of outputting the engine 
	//       configuration for use with the offline compiler asbuild.
	//       It should then be possible to execute pre-compiled bytecode.

	return 0;
}

int CompileScript(asIScriptEngine *engine, const char *scriptFile)
{
	int r;

	// We will only initialize the global variables once we're 
	// ready to execute, so disable the automatic initialization
	engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

	CScriptBuilder builder;
	r = builder.StartNewModule(engine, "script");
	if( r < 0 ) return -1;

	r = builder.AddSectionFromFile(scriptFile);
	if( r < 0 ) return -1;

	r = builder.BuildModule();
	if( r < 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Script failed to build");
		return -1;
	}

	return 0;
}

int ExecuteScript(asIScriptEngine *engine, const char *scriptFile, bool debug)
{
	CDebugger *dbg = 0;

	asIScriptModule *mod = engine->GetModule("script", asGM_ONLY_IF_EXISTS);
	if( !mod ) return -1;

	// Find the main function
	asIScriptFunction *func = mod->GetFunctionByDecl("int main()");
	if( func == 0 )
	{
		// Try again with "void main()"
		func = mod->GetFunctionByDecl("void main()");
	}

	if( func == 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Cannot find 'int main()' or 'void main()'");
		return -1;
	}

	// Set up a context to execute the script
	asIScriptContext *ctx = engine->CreateContext();

	if( debug )
	{
		cout << "Debugging, waiting for commands. Type 'h' for help." << endl;

		// Create the debugger instance
		dbg = new CDebugger();

		// Allow the user to initialize the debugging before moving on
		dbg->TakeCommands(ctx);
	}

	if( dbg )
	{
		// Set the line callback for the debugging
		ctx->SetLineCallback(asMETHOD(CDebugger, LineCallback), dbg, asCALL_THISCALL);
	}

	// Once we have the main function, we first need to initialize the global variables
	// Pass our own context so the initialization of the global variables can be debugged too
	int r = mod->ResetGlobalVars(ctx);
	if( r < 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Failed while initializing global variables");
		return -1;
	}

	// Execute the script
	ctx->Prepare(func);
	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
		{
			cout << "The script failed with an exception" << endl;
			PrintException(ctx, true);
			r = -1;
		}
		else if( r == asEXECUTION_ABORTED )
		{
			cout << "The script was aborted" << endl;
			r = -1;
		}
		else
		{
			cout << "The script terminated unexpectedly (" << r << ")" << endl;
			r = -1;
		}
	}
	else
	{
		// Get the return value from the script
		if( func->GetReturnTypeId() == asTYPEID_INT32 )
		{
			r = *(int*)ctx->GetAddressOfReturnValue();
		}
		else
			r = 0;
	}
	ctx->Release();

	// Destroy debugger
	if( dbg )
		delete dbg;

	return r;
}

// This little function allows the script to print a string to the screen
void PrintString(const string &str)
{
	cout << str;
}

// This function returns the command line arguments that were passed to the script
CScriptArray *GetCommandLineArgs()
{
	if( g_commandLineArgs )
	{
		g_commandLineArgs->AddRef();
		return g_commandLineArgs;
	}

    // Obtain a pointer to the engine
    asIScriptContext *ctx = asGetActiveContext();
    asIScriptEngine *engine = ctx->GetEngine();

    // Create the array object
	asIObjectType *arrayType = engine->GetObjectTypeById(engine->GetTypeIdByDecl("array<string>"));
    g_commandLineArgs = new CScriptArray(0, arrayType);

    // Find the existence of the delimiter in the input string
	for( int n = 0; n < g_argc; n++ )
	{
        // Add the arg to the array
        g_commandLineArgs->Resize(g_commandLineArgs->GetSize()+1);
        ((string*)g_commandLineArgs->At(n))->assign(g_argv[n]);
    }

    // Return the array by handle
	g_commandLineArgs->AddRef();
    return g_commandLineArgs;
}
