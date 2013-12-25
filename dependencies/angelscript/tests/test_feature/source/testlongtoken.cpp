//
// Tests to verify that long tokens doesn't crash the library
//

#include "utils.h"

static const char * const TESTNAME = "TestLongToken";


bool TestLongToken()
{
	bool fail = false;

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

	std::string str;

	str.resize(400);
	memset(&str[0], 'a', 400);
	str += " = 1";

	ExecuteString(engine, str.c_str());

	engine->Release();

	// Success
	return fail;
}
