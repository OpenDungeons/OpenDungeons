#include "utils.h"

namespace TestImplicitHandle
{


static const char *script1 =
"class@ MyClass                              \n" // Define the class to be an implicit handle type
"{                                           \n"
"    MyClass() { print('Created\\n'); }      \n"
"    int func() { return 7; }                \n"
"    string x;                               \n"
"};                                          \n"
"                                            \n"
"void testClass(MyClass x)                   \n" // Parameter pass by handles
"{                                           \n"
"    print(x.func()+'\\n');                  \n"
"}                                           \n"
"                                            \n"
"void main()                                 \n"
"{                                           \n"
"    MyClass p,p2;                           \n"
"                                            \n"
"    p = p2;                                 \n"
"                                            \n"
"    if (p is null) { print('Hello!\\n'); }  \n"
"                                            \n"
"    p = MyClass();                          \n"
"                                            \n"
"    print('---\\n');                        \n"
"    testClass(p);                           \n"
"    print('---\\n');                        \n"
"                                            \n"
"    print(p.func()+'\\n');                  \n"
"}                                           \n";

std::string output;
void print(std::string &str)
{
	output += str;
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		// Skipping this due to not supporting native calling conventions
		printf("Skipped due to AS_MAX_PORTABILITY\n");
		return false;
	}

	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->SetEngineProperty(asEP_ALLOW_IMPLICIT_HANDLE_TYPES, true);
	RegisterScriptString(engine);

	r = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(print), asCALL_CDECL); assert( r >= 0 );

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script1, strlen(script1), 0);
	r = mod->Build();
	if( r < 0 )
	{
		TEST_FAILED;
		printf("Failed to compile the script\n");
	}

	r = ExecuteString(engine, "main()", mod);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
		printf("Execution failed\n");
	}

	if( output != "Hello!\nCreated\n---\n7\n---\n7\n" )
	{
		TEST_FAILED;
		printf("Got: \n%s", output.c_str());
	}

	
	// TODO: The equality operator shouldn't perform handle comparison
	/*
	r = engine->ExecuteString(0, "MyClass a; assert( a == null );");
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	*/

	engine->Release();

	// Success
	return fail;
}

} // namespace

