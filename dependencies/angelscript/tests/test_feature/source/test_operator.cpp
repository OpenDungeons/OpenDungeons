#include "utils.h"

namespace TestOperator
{


class AppVal
{
public:
	AppVal() { value = 0; }
	int value;

	bool operator == (const AppVal &o) const
	{
		return value == o.value;
	}
};

void Construct(AppVal *a)
{
	new(a) AppVal();
}

AppVal g_AppVal;
AppVal &GetAppValRef(AppVal &a)
{
	a.value = 1;
	return g_AppVal;
}

bool Test()
{
	bool fail = false;
	CBufferedOutStream bout;
	int r;
	asIScriptEngine *engine = 0;
	asIScriptModule *mod = 0;

	//----------------------------------------------
	// opEquals for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operator ==
			"  bool opEquals(const Test &in o) const \n"
			"  {                                     \n"
			"    return value == o.value;            \n"
			"  }                                     \n"
			// The operator can be overloaded for different types
			"  bool opEquals(int o)             \n"
			"  {                                \n"
			"    return value == o;             \n"
			"  }                                \n"
			// opEquals that don't return bool are ignored
			"  int opEquals(float o)            \n"
			"  {                                \n"
			"    return 0;                      \n"
			"  }                                \n"
			"}                                  \n"
			"Test func()                        \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return a;                        \n"
			"}                                  \n"
			"Test @funcH()                      \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return @a;                       \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test a,b,c;                      \n"
			"  a.value = 0;                     \n"
			"  b.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  assert( a == b );                \n"  // a.opEquals(b)
			"  assert( a.opEquals(b) );         \n"  // Same as a == b
			"  assert( a == 0 );                \n"  // a.opEquals(0)
			"  assert( 0 == a );                \n"  // a.opEquals(0)
			"  assert( a == 0.1f );             \n"  // a.opEquals(int(0.1f))
			"  assert( a != c );                \n"  // !a.opEquals(c)
			"  assert( a != 1 );                \n"  // !a.opEquals(1)
			"  assert( 1 != a );                \n"  // !a.opEquals(1)
			"  assert( !a.opEquals(c) );        \n"  // Same as a != c
			"  assert( a == func() );           \n"
			"  assert( a == funcH() );          \n"
			"  assert( func() == a );           \n"
			"  assert( funcH() == a );          \n"
			"}                                  \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		if( bout.buffer != "script (29, 1) : Info    : Compiling void main()\n"
		                   "script (39, 16) : Warning : Implicit conversion of value is not exact\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		// Test const correctness. opEquals(int) isn't const so it must not be allowed
		bout.buffer = "";
		r = ExecuteString(engine, "Test a; const Test @h = a; assert( h == 0 );", mod);
		if( r >= 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "ExecuteString (1, 38) : Error   : No conversion from 'const Test@&' to 'int' available.\n" )
		{
			printf("%s", bout.buffer.c_str());
		}

		engine->Release();
	}

	//--------------------------------------------
	// opEquals for application classes
	//
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") == 0 )
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = engine->RegisterObjectType("AppVal", sizeof(AppVal), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS); assert( r >= 0 );
		r = engine->RegisterObjectBehaviour("AppVal", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Construct, (AppVal*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
		r = engine->RegisterObjectMethod("AppVal", "bool opEquals(const AppVal &in) const", asMETHODPR(AppVal, operator ==, (const AppVal &) const, bool), asCALL_THISCALL); assert( r >= 0 );
		r = engine->RegisterObjectProperty("AppVal", "int value", asOFFSET(AppVal, value)); assert( r >= 0 );

		r = engine->RegisterGlobalFunction("AppVal &GetAppValRef(AppVal &out)", asFUNCTIONPR(GetAppValRef, (AppVal &), AppVal &), asCALL_CDECL); assert( r >= 0 );
		g_AppVal.value = 0;

		const char *script = 			
			"void main()                        \n"
			"{                                  \n"
			"  AppVal a,b,c;                    \n"
			"  a.value = 0;                     \n"
			"  b.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  assert( a == b );                \n"  // a.opEquals(b)
			"  assert( a.opEquals(b) );         \n"  // Same as a == b
			"  assert( a != c );                \n"  // !a.opEquals(c)
			"  assert( !a.opEquals(c) );        \n"  // Same as a != c
			"  assert( a == GetAppValRef(b) );  \n"
			"  assert( b == c );                \n"
			"  b.value = 0;                     \n"
			"  assert( GetAppValRef(b) == a );  \n"
			"  assert( c == b );                \n"
			"  assert( AppVal() == a );         \n"
			"}                                  \n";

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
		}

		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
		}

		engine->Release();
	}

	//----------------------------------------------
	// opCmp for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operators ==, !=, <, <=, >, >=
			"  int opCmp(const Test &in o) const  \n"
			"  {                                  \n"
			"    return value - o.value;          \n"
			"  }                                  \n"
			// The operator can be overloaded for different types
			"  int opCmp(int o)                 \n"
			"  {                                \n"
			"    return value - o;              \n"
			"  }                                \n"
			// opCmp that don't return int are ignored
			"  bool opCmp(float o)              \n"
			"  {                                \n"
			"    return false;                  \n"
			"  }                                \n"
			"}                                  \n"
			"Test func()                        \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return a;                        \n"
			"}                                  \n"
			"Test @funcH()                      \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 0;                     \n"
			"  return @a;                       \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test a,b,c;                      \n"
			"  a.value = 0;                     \n"
			"  b.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  assert( a == b );                \n"  // a.opCmp(b) == 0
			"  assert( a.opCmp(b) == 0 );       \n"  // Same as a == b
			"  assert( a == 0 );                \n"  // a.opCmp(0) == 0
			"  assert( 0 == a );                \n"  // a.opCmp(0) == 0
			"  assert( a == 0.1f );             \n"  // a.opCmp(int(0.1f) == 0 )
			"  assert( a != c );                \n"  // a.opCmp(c) != 0
			"  assert( a != 1 );                \n"  // a.opCmp(1) != 0
			"  assert( 1 != a );                \n"  // a.opCmp(1) != 0
			"  assert( a.opCmp(c) != 0 );       \n"  // Same as a != c
			"  assert( a == func() );           \n"
			"  assert( a == funcH() );          \n"
			"  assert( func() == a );           \n"
			"  assert( funcH() == a );          \n"
			"  assert( a < 10 );                \n"
			"  assert( 10 > a );                \n"
			"  assert( c > 0 );                 \n"
			"  assert( 0 < c );                 \n"
			"}                                  \n";

		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
		}
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
		}

		// Test const correctness. opCmp(int) isn't const so it must not be allowed
		bout.buffer = "";
		r = ExecuteString(engine, "Test a; const Test @h = a; assert( h == 0 );", mod);
		if( r >= 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "ExecuteString (1, 38) : Error   : No conversion from 'const Test@&' to 'int' available.\n" )
		{
			printf("%s", bout.buffer.c_str());
		}

		engine->Release();
	}

	//----------------------------------------------
	// Other dual operators for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operators 
			"  Test opAdd(const Test &in o) const \n" // ordinary operator
			"  {                                  \n"
			"    Test t;                          \n"
			"    t.value = value + o.value;       \n"
			"    return t;                        \n"
			"  }                                  \n"
			"  Test opMul_r(int o) const          \n" // reversed order arguments
			"  {                                  \n"
			"    Test t;                          \n"
			"    t.value = o * value;             \n"
			"    return t;                        \n"
			"  }                                  \n"
			"  Test @opShl(int o)                 \n" // Implementing a stream operator << 
			"  {                                  \n"
			"    value += o;                      \n"
			"    return this;                     \n"
			"  }                                  \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test c;                          \n"
			"  c.value = 1;                     \n"
			"  assert( (c + c).value == 2 );      \n"  // c.opAdd(c).value == 2
			"  assert( c.opAdd(c).value == 2 );   \n"
			"  assert( (3 * c).value == 3 );      \n"  // c.opMul_r(3).value == 3
			"  assert( c.opMul_r(3).value == 3 ); \n"
			"  c << 1 << 2 << 3;                \n"
			"  assert( c.value == 7 );          \n"
			"}                                  \n";

		bout.buffer = "";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
		}

		engine->Release();
	}

	//----------------------------------------------
	// Assignment operators for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operators 
			"  Test@ opAssign(const Test &in o)   \n" 
			"  {                                  \n"
			"    value = o.value;                 \n"
			"    return this;                     \n"
			"  }                                  \n"
			"  Test@ opMulAssign(int o)           \n" 
			"  {                                  \n"
			"    value *= o;                      \n"
			"    return this;                     \n"
			"  }                                  \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test a,c;                        \n"
			"  a.value = 0;                     \n"
			"  c.value = 1;                     \n"
			"  a = c;                           \n"
			"  assert( a.value == 1 );          \n"
			"  a.value = 2;                     \n"
			"  a *= 2;                          \n"
			"  assert( a.value == 4 );          \n"
			"}                                  \n";

		bout.buffer = "";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
		}

		engine->Release();
	}

	//----------------------------------------------
	// Unary operators for script classes
	//
	{
 		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

		const char *script = 
			"class Test                         \n"
			"{                                  \n"
			"  int value;                       \n"
			// Define the operators 
			"  Test opNeg() const               \n"
			"  {                                \n"
			"    Test t;                        \n"
			"    t.value = -value;              \n"
			"    return t;                      \n"
			"  }                                \n"
			"  Test opCom()                     \n"
			"  {                                \n"
			"    Test t;                        \n"
			"    t.value = ~value;              \n"
			"    return t;                      \n"
			"  }                                \n"
			"  void opPostInc()                 \n"
			"  {                                \n"
			"    value++;                       \n"
			"  }                                \n"
			"  void opPreDec()                  \n"
			"  {                                \n"
			"    --value;                       \n"
			"  }                                \n"
			"}                                  \n"
			"void main()                        \n"
			"{                                  \n"
			"  Test a;                          \n"
			"  a.value = 1;                     \n"
			"  assert( (-a).value == -1 );      \n"
			"  assert( (~a).value == int(~1) ); \n"
			"  a++;                             \n"
			"  assert( a.value == 2 );          \n"
			"  --a;                             \n"
			"  assert( a.value == 1 );          \n"
			"}                                  \n";

		bout.buffer = "";
		mod->AddScriptSection("script", script);
		r = mod->Build();
		if( r < 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "" )
		{
			printf("%s", bout.buffer.c_str());
		}
		
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
		{
			TEST_FAILED;
		}

		// Test const correctness.
		bout.buffer = "";
		r = ExecuteString(engine, "Test a; const Test @h = a; assert( (~h).value == ~1 ); h++; --h;", mod);
		if( r >= 0 )
		{
			TEST_FAILED;
		}
		if( bout.buffer != "ExecuteString (1, 37) : Error   : Function 'opCom() const' not found\n"
			               "ExecuteString (1, 57) : Error   : Function 'opPostInc() const' not found\n"
		                   "ExecuteString (1, 61) : Error   : Function 'opPreDec() const' not found\n" )
		{
			printf("%s", bout.buffer.c_str());
		}

		engine->Release();
	}

	// Success
	return fail;
}

}
