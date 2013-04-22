#include "utils.h"

namespace TestFloat
{

static const char * const TESTNAME = "TestFloat";



static const char *script =
"void TestFloat()                               \n"
"{                                              \n"
"  float a = 2, b = 3, c = 1;                   \n"
"  c = a + b;                                   \n"
"  a = b + 23;                                  \n"
"  b = 12 + c;                                  \n"
"  c = a - b;                                   \n"
"  a = b - 23;                                  \n"
"  b = 12 - c;                                  \n"
"  c = a * b;                                   \n"
"  a = b * 23;                                  \n"
"  b = 12 * c;                                  \n"
"  c = a / b;                                   \n"
"  a = b / 23;                                  \n"
"  b = 12 / c;                                  \n"
"  c = a % b;                                   \n"
"  a = b % 23;                                  \n"
"  b = 12 % c;                                  \n"
"  a++;                                         \n"
"  ++a;                                         \n"
"  a += b;                                      \n"
"  a += 3;                                      \n"
"  a /= c;                                      \n"
"  a /= 5;                                      \n"
"  a = b = c;                                   \n"
"  func( a-1, b, c );                           \n"
"  a = -b;                                      \n"
"  a = func2();                                 \n"
"}                                              \n"
"void func(float a, float &in b, float &out c)  \n"
"{                                              \n"
"  c = a + b;                                   \n"
"  b = c;                                       \n"
"  g = g;                                       \n"
"}                                              \n"
"float g = 0;                                   \n"
"float func2()                                  \n"
"{                                              \n"
"  return g + 1;                                \n"
"}                                              \n";


static const char *script2 =
"void start()           \n"
"{                      \n"
"float test = 1.9f;     \n"
"print(test);           \n"
"print(test + test);    \n"
"}                      \n";

void print_gen(asIScriptGeneric *gen)
{
	float val = *(float*)gen->GetAddressOfArg(0);
	UNUSED_VAR(val);
}


bool Test()
{
	bool fail = false;
	COutStream out;
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
 	int r = mod->Build();
	if( r < 0 ) TEST_FAILED;


	engine->RegisterGlobalFunction("void print(float)", asFUNCTION(print_gen), asCALL_GENERIC);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;

	ExecuteString(engine, "start()", mod);

	// The locale affects the way the compiler reads float values
	setlocale(LC_NUMERIC, "");

	float f;
	engine->RegisterGlobalProperty("float f", &f);
	r = ExecuteString(engine, "f = 3.14f;");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	if( f < 3.139999f || f > 3.140001f )
		TEST_FAILED;

	setlocale(LC_NUMERIC, "C");

	engine->Release();

	if( fail )
		printf("%s: failed\n", TESTNAME);

	// Success
 	return fail;
}

} // namespace

