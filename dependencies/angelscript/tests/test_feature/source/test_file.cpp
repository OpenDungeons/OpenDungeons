#include "utils.h"
#include "../../../add_on/scriptfile/scriptfile.h"

namespace TestFile
{

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
	RegisterScriptString(engine);
	RegisterScriptFile(engine);

	const char *script =
		"file f;                                                  \n"
		"int r = f.open(\"scripts/TestExecuteScript.as\", \"r\"); \n"
		"if( r >= 0 ) {                                           \n"
		"  assert( f.getSize() > 0 );                             \n"
		"  string s1; f.readString(10000, s1);                    \n"
		"  assert( s1.length() == uint(f.getSize()) );            \n"
		"  f.close();                                             \n"
		"  f.open('scripts/TestExecuteScript.as', 'r');           \n"
		"  string s2;                                             \n"
		"  while( !f.isEndOfFile() )                              \n"
		"  {                                                      \n"
		"    string s3; f.readLine(s3);                           \n"
		"    s2 += s3;                                            \n"
		"  }                                                      \n"
		"  assert( s1 == s2 );                                    \n"
		"  f.close();                                             \n"
		"}                                                        \n";

	r = ExecuteString(engine, script);
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	engine->Release();

	// Success
	return fail;
}

} // namespace

