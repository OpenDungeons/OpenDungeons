#include "utils.h"

#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif
#include "../../../add_on/scriptbuilder/scriptbuilder.h"
#include "../../../add_on/scriptmath/scriptmathcomplex.h"

namespace Test_Addon_ScriptBuilder
{

const char *script =
// Global functions can have meta data
"[ my meta data test ] void func1() {} \n"
// meta data strings can contain any tokens, and can use nested []
"[ test['hello'] ] void func2() {} \n"
// global variables can have meta data
"[ init ] int g_var = 0; \n"
// Parts of the code can be excluded through conditional compilation
"#if DONTCOMPILE                                      \n"
"  This code should be excluded by the CScriptBuilder \n"
"  #if NESTED                                         \n"
"    Nested blocks are also possible                  \n"
"  #endif                                             \n"
"  Nested block ended                                 \n"
"#endif                                               \n"
// global object variable
"[ var of type myclass ] MyClass g_obj(); \n"
// class declarations can have meta data
"#if COMPILE \n"
"[ myclass ] class MyClass {} \n"
" #if NESTED \n"
"   dont compile this nested block \n"
" #endif \n"
"#endif \n"
// class properties can also have meta data
"[ myclass2 ] \n"
"class MyClass2 { \n"
" [ edit ] \n"
" int a; \n"
" int func() { \n"
"   return 0; \n"
" } \n"
" [ noedit ] int b; \n"
" [ edit,c ] \n"
" complex c; \n"
" [ prop ] \n" // It's possible to inform meta data for virtual properties too
" complex prop { get {return c;} set {c = value;} } \n"
"} \n"
// interface declarations can have meta data
"[ myintf ] interface MyIntf {} \n"
// arrays must still work
"int[] arr = {1, 2, 3}; \n"
"int[] @arrayfunc(int[] @a) { a.resize(1); return a; } \n"
// directives in comments should be ignored
"/* \n"
"#include \"dont_include\" \n"
"*/ \n"
// namespaces can also contain entities with metadata
"namespace NS { \n"
" [func] void func() {} \n"
" [class] class Class {} \n"
"} \n"
;

using namespace std;



bool Test()
{
	bool fail = false;
	int r = 0;
	COutStream out;
	CBufferedOutStream bout;

	// TODO: Preprocessor directives should be alone on the line

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	RegisterScriptArray(engine, true);

	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	if( !strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
		RegisterScriptMathComplex(engine);
	else
		engine->RegisterObjectType("complex", 4, asOBJ_VALUE | asOBJ_POD);

	// Test the parse token method
	asETokenClass t = engine->ParseToken("!is");
	if( t != asTC_KEYWORD )
		TEST_FAILED;

	// Compile a script with meta data strings
	CScriptBuilder builder;
	builder.DefineWord("COMPILE");
	r = builder.StartNewModule(engine, 0);
	r = builder.AddSectionFromMemory("", script);
	r = builder.BuildModule();
#if AS_PROCESS_METADATA == 1
	if( r < 0 )
		TEST_FAILED;

	asIScriptFunction *func = engine->GetModule(0)->GetFunctionByName("func1");
	string metadata = builder.GetMetadataStringForFunc(func);
	if( metadata != " my meta data test " )
		TEST_FAILED;

	func = engine->GetModule(0)->GetFunctionByName("func2");
	metadata = builder.GetMetadataStringForFunc(func);
	if( metadata != " test['hello'] " )
		TEST_FAILED;

	engine->GetModule(0)->SetDefaultNamespace("NS");
	func = engine->GetModule(0)->GetFunctionByName("func");
	metadata = builder.GetMetadataStringForFunc(func);
	if( metadata != "func" )
		TEST_FAILED;
	engine->GetModule(0)->SetDefaultNamespace("");

	int typeId = engine->GetModule(0)->GetTypeIdByDecl("MyClass");
	metadata = builder.GetMetadataStringForType(typeId);
	if( metadata != " myclass " )
		TEST_FAILED;

	typeId = engine->GetModule(0)->GetTypeIdByDecl("NS::Class");
	metadata = builder.GetMetadataStringForType(typeId);
	if( metadata != "class" )
		TEST_FAILED;

	typeId = engine->GetModule(0)->GetTypeIdByDecl("MyClass2");
	metadata = builder.GetMetadataStringForTypeProperty(typeId, 0);
	if( metadata != " edit " )
		TEST_FAILED;
	metadata = builder.GetMetadataStringForTypeProperty(typeId, 1);
	if( metadata != " noedit " )
		TEST_FAILED;
	metadata = builder.GetMetadataStringForTypeProperty(typeId, 2);
	if( metadata != " edit,c " )
		TEST_FAILED;

	asIObjectType *type = engine->GetObjectTypeById(typeId);
	if( type == 0 )
		TEST_FAILED;
	else
	{
		metadata = builder.GetMetadataStringForTypeMethod(typeId, type->GetMethodByName("get_prop"));
		if( metadata != " prop " )
			TEST_FAILED;
		metadata = builder.GetMetadataStringForTypeMethod(typeId, type->GetMethodByName("set_prop"));
		if( metadata != " prop " )
			TEST_FAILED;
	}

	typeId = engine->GetModule(0)->GetTypeIdByDecl("MyIntf");
	metadata = builder.GetMetadataStringForType(typeId);
	if( metadata != " myintf " )
		TEST_FAILED;

	int varIdx = engine->GetModule(0)->GetGlobalVarIndexByName("g_var");
	metadata = builder.GetMetadataStringForVar(varIdx);
	if( metadata != " init " )
		TEST_FAILED;

	varIdx = engine->GetModule(0)->GetGlobalVarIndexByName("g_obj");
	metadata = builder.GetMetadataStringForVar(varIdx);
	if( metadata != " var of type myclass " )
		TEST_FAILED;
#endif

	// http://www.gamedev.net/topic/624445-cscriptbuilder-asset-string-subscript-out-of-range/
	{
		bout.buffer = "";
		CScriptBuilder builder;
		builder.StartNewModule(engine, "mod");
		builder.AddSectionFromMemory("", "#");
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = builder.BuildModule();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != " (1, 1) : Error   : Unexpected token '<unrecognized token>'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	// Add a script section from memory with length
	{
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &bout, asCALL_THISCALL);
		CScriptBuilder builder;
		builder.StartNewModule(engine, "mod");
		builder.AddSectionFromMemory("", "void func() {} $#", 14);
		r = builder.BuildModule();
		if( r < 0 )
			TEST_FAILED;
	}

	// http://www.gamedev.net/topic/631848-cscriptbuilder-bug/
	{
		bout.buffer = "";
		CScriptBuilder builder;
		builder.StartNewModule(engine, "mod");
		builder.AddSectionFromMemory("", "class ");
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		r = builder.BuildModule();
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer != " (1, 7) : Error   : Expected identifier\n"
		                   " (1, 7) : Error   : Expected '{'\n" )
		{
			printf("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	engine->Release();

	return fail;
}

} // namespace

