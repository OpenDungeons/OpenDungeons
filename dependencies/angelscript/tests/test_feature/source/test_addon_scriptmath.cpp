#include "utils.h"
#include "../../../add_on/scriptmath/scriptmath.h"
#include "../../../add_on/scriptmath/scriptmathcomplex.h"

namespace Test_Addon_ScriptMath
{

static const char * const TESTNAME = "TestAddonScriptMath";

static const char *script =
"complex TestComplex()  \n"
"{                      \n"
"  complex v;           \n"
"  v.r=1;               \n"
"  v.i=2;               \n"
"  return v;            \n"
"}                      \n"
"complex TestComplexVal(complex v)  \n"
"{                                  \n"
"  return v;                        \n"
"}                                  \n"
"void TestComplexRef(complex &out v)\n"
"{                                  \n"
"  v.r=1;                           \n"
"  v.i=2;                           \n"
"}                                  \n";

bool Test()
{
	bool fail = false;

	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("Skipped due to max portability\n");
		return fail;
	}

	COutStream out;
	CBufferedOutStream bout;
	int r;
	asIScriptEngine *engine = 0;


	engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	RegisterScriptMath(engine);
	RegisterScriptMathComplex(engine);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	Complex v;
	engine->RegisterGlobalProperty("complex v", &v);

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
		r = ExecuteString(engine, "v = TestComplex();", mod);
		if( r < 0 )
		{
			printf("%s: ExecuteString() failed %d\n", TESTNAME, r);
			TEST_FAILED;
		}
		if( v.r != 1 || v.i != 2 )
		{
			printf("%s: Failed to assign correct Complex\n", TESTNAME);
			TEST_FAILED;
		}

		// Manual return
		v.r = 0; v.i = 0;

		asIScriptContext *ctx = engine->CreateContext();
		ctx->Prepare(mod->GetFunctionByDecl("complex TestComplex()"));

		ctx->Execute();
		Complex *ret = (Complex*)ctx->GetReturnObject();
		if( ret->r != 1 || ret->i != 2 )
		{
			printf("%s: Failed to assign correct Complex\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Prepare(mod->GetFunctionByDecl("complex TestComplexVal(complex)"));
		v.r = 3; v.i = 2;
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		ret = (Complex*)ctx->GetReturnObject();
		if( ret->r != 3 || ret->i != 2 )
		{
			printf("%s: Failed to pass Complex by val\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Prepare(mod->GetFunctionByDecl("void TestComplexRef(complex &out)"));
		ctx->SetArgObject(0, &v);
		ctx->Execute();
		if( v.r != 1 || v.i != 2 )
		{
			printf("%s: Failed to pass Complex by ref\n", TESTNAME);
			TEST_FAILED;
		}

		ctx->Release();
	}

	// Assignment of temporary object
	r = ExecuteString(engine, "complex v; \n"
		                      "float r = (v = complex(10.0f,7)).r; \n"
							  "assert( r > 9.9999f && r < 10.0001f );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test some operator overloads
	r = ExecuteString(engine, "complex v(1,0); assert( (v*complex(2,0)).abs() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "complex v(1,0); assert( (complex(2,0)*v).abs() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "complex v(1,0); assert( (v+v).abs() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "complex v(1,0); assert( v == complex(1,0) );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	r = ExecuteString(engine, "complex v(1,0); assert( (v *= complex(2)).abs() == 2 );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test that implicit conversion to the complex type works
	r = ExecuteString(engine, "complex v(1,1); v += 2; assert( v == complex(3,1) ); v = v - 3; assert( abs(v.r - 0) < 0.0001f );");
	if( r != asEXECUTION_FINISHED )
	{
		TEST_FAILED;
	}

	// Test error message when constructor is not found
	bout.buffer = "";
	engine->SetMessageCallback(asMETHOD(CBufferedOutStream,Callback), &bout, asCALL_THISCALL);
	r = ExecuteString(engine, "complex v = complex(4,3,2,1);");
	if( r >= 0 )
	{
		TEST_FAILED;
	}
	// TODO: the function signature for the constructors/factories should carry the name of the object instead of _beh_0_
	if( bout.buffer != "ExecuteString (1, 13) : Error   : No matching signatures to 'complex(const int, const int, const int, const int)'\n"
					   "ExecuteString (1, 13) : Info    : Candidates are:\n"
					   "ExecuteString (1, 13) : Info    : void complex::_beh_0_()\n"
				   	   "ExecuteString (1, 13) : Info    : void complex::_beh_0_(const complex&in)\n"
					   "ExecuteString (1, 13) : Info    : void complex::_beh_0_(float)\n"
					   "ExecuteString (1, 13) : Info    : void complex::_beh_0_(float, float)\n" )
	{
		printf("%s", bout.buffer.c_str());
		TEST_FAILED;
	}

	engine->Release();

	return fail;
}

}

