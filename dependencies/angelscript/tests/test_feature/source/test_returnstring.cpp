#include "utils.h"
#include <string>
#include <iostream>

namespace TestReturnString
{

static const char * const TESTNAME = "TestReturnString";


struct Foo 
{
	Foo() {}
	~Foo() {}
	Foo(const Foo& /*rhs*/) {}
	Foo& operator=(const Foo& /*rhs*/) { return *this; }
};

//THIS CRASHES.
std::string foo_member_fun_one(const std::string& in, Foo* thisp)
{
	assert(in == "foo");
	return in;
}

void foo_member_fun_two(const std::string& in, Foo* thisp)
{
	assert(in == "foo");
}

std::string free_fun(const std::string& in)
{
	assert(in == "foo");
	return in;
}

void ConstructFoo(Foo* ptr) { new (ptr) Foo(); }
void CopyConstructFoo(const Foo& rhs, Foo* ptr) { new (ptr) Foo(rhs); }
void DestroyFoo(Foo* ptr) { ptr->~Foo(); } 
Foo& AssignFoo(const Foo& rhs, Foo* ptr) { return (*ptr) = rhs; }

void ConstructString(std::string* ptr) { new (ptr) std::string(); }
void CopyConstructString(const std::string& rhs, std::string* ptr) { new (ptr) std::string(rhs); }
void DestroyString(std::string* ptr)
{
#if !defined(__BORLANDC__) || __BORLANDC__ >= 0x590
	// Some weird BCC bug (which was fixed in C++Builder 2007) prevents us from calling a
	// destructor explicitly on template functions.
	ptr->~basic_string();
#endif
}
std::string& AssignString(const std::string& rhs, std::string* ptr) { return (*ptr) = rhs; }

std::string StringFactory(unsigned int length, const char *s)
{
	return std::string(s,length);
}

bool Test()
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		printf("%s: Skipped due to AS_MAX_PORTABILITY\n", TESTNAME);
		return false;
	}

	asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetEngineProperty(asEP_ALLOW_UNSAFE_REFERENCES, 1);

	int r = 0;
	
	r = engine->RegisterObjectType("Foo",sizeof(Foo),asOBJ_VALUE | asOBJ_APP_CLASS_CDA); assert( r >=0 );

	r = engine->RegisterObjectBehaviour("Foo",
		asBEHAVE_CONSTRUCT,
		"void constructor()",
		asFUNCTION(ConstructFoo),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectBehaviour("Foo",
		asBEHAVE_DESTRUCT,
		"void destructor()",
		asFUNCTION(DestroyFoo),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectMethod("Foo",
		"Foo& opAssign(const Foo&)",
		asFUNCTION(AssignFoo),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectBehaviour("Foo",
		asBEHAVE_CONSTRUCT,
		"void constructor(const Foo&)",
		asFUNCTION(CopyConstructFoo),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );				

	r = engine->RegisterObjectType("string",sizeof(std::string),asOBJ_VALUE | asOBJ_APP_CLASS_CDA);assert( r >=0 );

	r = engine->RegisterObjectBehaviour("string",
		asBEHAVE_CONSTRUCT,
		"void constructor()",
		asFUNCTION(ConstructString),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectBehaviour("string",
		asBEHAVE_DESTRUCT,
		"void destructor()",
		asFUNCTION(DestroyString),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectMethod("string",
		"string& opAssign(const string&)",
		asFUNCTION(AssignString),
		asCALL_CDECL_OBJLAST);	assert( r >=0 );

	r = engine->RegisterObjectBehaviour("string",
		asBEHAVE_CONSTRUCT,
		"void constructor(const string&)",
		asFUNCTION(CopyConstructString),
		asCALL_CDECL_OBJLAST);		assert( r >=0 );			

	r = engine->RegisterStringFactory("string",
		asFUNCTION(StringFactory),
		asCALL_CDECL);assert( r >=0 );

	r = engine->RegisterObjectMethod("Foo",
		"string member_one(const string&)",
		asFUNCTION(foo_member_fun_one),
		asCALL_CDECL_OBJLAST);assert( r >=0 );

	r = engine->RegisterObjectMethod("Foo",
		"void member_two(const string&)",
		asFUNCTION(foo_member_fun_two),
		asCALL_CDECL_OBJLAST);assert( r >=0 );

	r = engine->RegisterGlobalFunction(
		"string free_fun(const string&)",
		asFUNCTION(free_fun),
		asCALL_CDECL);assert( r >=0 );

	std::string script = "void test(Foo f) { free_fun(\"foo\"); f.member_two(\"foo\"); f.member_one(\"foo\"); }";
	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test",script.c_str(),script.length());
	
	r = mod->Build();
	if (r < 0) std::cout << "Error: " << r;

	asIScriptFunction *func = engine->GetModule(0)->GetFunctionByName("test");
	if (func == 0) std::cout << r;

	asIScriptContext* ctx = engine->CreateContext();
	r = ctx->Prepare(func);
	if (r < 0) std::cout << "Error: " << r;

	Foo f;

	r = ctx->SetArgObject(0,&f);
	if (r < 0) std::cout << "Error: " << r;

	r = ctx->Execute();
	if (r != asEXECUTION_FINISHED) std::cout << "Error: " << r;

	ctx->Release();

	//------------------------

	script = "void test(Foo f) { string s = f.member_one(\"foo\"); s = \"test\"; }";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script.c_str(), script.length());

	r = mod->Build();
	if( r < 0 ) std::cout << "Error: " << r;

	func = engine->GetModule(0)->GetFunctionByName("test");
	if( func == 0 ) std::cout << r;

	ctx = engine->CreateContext();
	r = ctx->Prepare(func);
	if( r < 0 ) std::cout << "Error: " << r;

	r = ctx->SetArgObject(0,&f);
	if( r < 0 ) std::cout << "Error: " << r;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED) std::cout << "Error: " << r;

	ctx->Release();

	//-------------------------
	script = "void test() { Func(\"test\"); } void Func(const string &in str) {}";
	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("test", script.c_str(), script.length());

	r = mod->Build();
	if( r < 0 ) std::cout << "Error: " << r;

	func = engine->GetModule(0)->GetFunctionByName("test");
	if( func == 0 ) std::cout << r;

	ctx = engine->CreateContext();
	r = ctx->Prepare(func);
	if( r < 0 ) std::cout << "Error: " << r;

	r = ctx->Execute();
	if( r != asEXECUTION_FINISHED) std::cout << "Error: " << r;

	ctx->Release();

	engine->Release();
	

	return 0;
}




} // namespace

