#include "utils.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace TestDump
{

void DumpModule(asIScriptModule *mod);

bool Test()
{
	bool fail = false;
	int r;
	COutStream out;

	const char *script = 
		"void Test() {} \n"
		"class A : I { void i(float) {} void a(int) {} float f; } \n"
		"class B : A { B(int) {} } \n"
		"interface I { void i(float); } \n"
		"float a; \n"
		"const float aConst = 3.141592f; \n"
		"I@ i; \n"
		"enum E { eval = 0, eval2 = 2 } \n"
		"E e; \n"
		"typedef float real; \n"
		"real pi = 3.14f; \n"
		"import void ImpFunc() from \"mod\"; \n";

	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

	RegisterScriptArray(engine, true);
	RegisterStdString(engine);

	float f;
	engine->RegisterTypedef("myFloat", "float");
	engine->RegisterGlobalProperty("myFloat f", &f);
	engine->RegisterGlobalProperty("const float myConst", &f);
	engine->RegisterGlobalFunction("void func(int &in)", asFUNCTION(0), asCALL_GENERIC);

	engine->BeginConfigGroup("test");
	engine->RegisterGlobalFunction("void func2()", asFUNCTION(0), asCALL_GENERIC);
	engine->EndConfigGroup();

	engine->RegisterEnum("myEnum");
	engine->RegisterEnumValue("myEnum", "value1", 1);
	engine->RegisterEnumValue("myEnum", "value2", 2);

	engine->RegisterFuncdef("void Callback(int a, int b)");

	engine->RegisterInterface("MyIntf");
	engine->RegisterInterfaceMethod("MyIntf", "void func() const");

	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);

	mod->AddScriptSection("script", script);
	r = mod->Build();
	if( r < 0 )
		TEST_FAILED;

	WriteConfigToFile(engine, "AS_DEBUG/config.txt");

	DumpModule(mod);

	// Save/Restore the bytecode and then test again for the loaded bytecode
	CBytecodeStream stream(__FILE__"1");
	mod->SaveByteCode(&stream);

	mod = engine->GetModule("2", asGM_ALWAYS_CREATE);
	mod->LoadByteCode(&stream);

	DumpModule(mod);

	engine->Release();

	return fail;
}

void DumpObjectType(stringstream &s, asIObjectType *objType)
{
	asIScriptEngine *engine = objType->GetEngine();

	if( objType->GetFlags() & asOBJ_SCRIPT_OBJECT )
	{
		if( objType->GetSize() ) 
		{
			string inheritance;
			if( objType->GetBaseType() )
				inheritance += objType->GetBaseType()->GetName();

			for( asUINT i = 0; i < objType->GetInterfaceCount(); i++ )
			{
				if( inheritance.length() )
					inheritance += ", ";
				inheritance += objType->GetInterface(i)->GetName();
			}

			s << "type: class " << objType->GetName() << " : " << inheritance << endl;
		}
		else
		{
			s << "type: interface " << objType->GetName() << endl;
		}
	}
	else
	{
		s << "reg type: ";
		if( objType->GetFlags() & asOBJ_REF )
			s << "ref ";
		else
			s << "val ";

		s << objType->GetName();

		const char *group = objType->GetConfigGroup();
		s << " group: " << (group ? group : "<null>") << endl;
	}

	// Show factory functions
	for( asUINT f = 0; f < objType->GetFactoryCount(); f++ )
	{
		s << " " << objType->GetFactoryByIndex(f)->GetDeclaration() << endl;
	}

	if( !( objType->GetFlags() & asOBJ_SCRIPT_OBJECT ) )
	{
		// Show behaviours
		for( asUINT b = 0; b < objType->GetBehaviourCount(); b++ )
		{
			asEBehaviours beh;
			asIScriptFunction *bid = objType->GetBehaviourByIndex(b, &beh);
			s << " beh(" << beh << ") " << bid->GetDeclaration(false) << endl;
		}
	}

	// Show methods
	for( asUINT m = 0; m < objType->GetMethodCount(); m++ )
	{
		s << " " << objType->GetMethodByIndex(m)->GetDeclaration(false) << endl;
	}

	// Show properties
	for( asUINT p = 0; p < objType->GetPropertyCount(); p++ )
	{
		s << " " << objType->GetPropertyDeclaration(p) << endl;
	}
}

void DumpModule(asIScriptModule *mod)
{
	int c, n;
	asIScriptEngine *engine = mod->GetEngine();
	stringstream s;

	// Enumerate global functions
	c = mod->GetFunctionCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *func = mod->GetFunctionByIndex(n);
		s << "func: " << func->GetDeclaration() << endl;
	}

	// Enumerate object types
	c = mod->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		DumpObjectType(s, mod->GetObjectTypeByIndex(n));
	}

	// Enumerate global variables
	c = mod->GetGlobalVarCount();
	for( n = 0; n < c; n++ )
	{
		s << "global: " << mod->GetGlobalVarDeclaration(n) << endl;
	}

	// Enumerate enums
	c = mod->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int eid;
		const char *ename = mod->GetEnumByIndex(n, &eid);

		s << "enum: " << ename << endl;

		// List enum values
		for( int e = 0; e < mod->GetEnumValueCount(eid); e++ )
		{
			int value;
			const char *name = mod->GetEnumValueByIndex(eid, e, &value);
			s << " " << name << " = " << value << endl;
		}
	}

	// Enumerate type defs
	c = mod->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int tid;
		const char *name = mod->GetTypedefByIndex(n, &tid);

		s << "typedef: " << name << " => " << engine->GetTypeDeclaration(tid) << endl;
	}

	// Enumerate imported functions
	c = mod->GetImportedFunctionCount();
	for( n = 0; n < c; n++ )
	{
		s << "import: " << mod->GetImportedFunctionDeclaration(n) << " from \"" << mod->GetImportedFunctionSourceModule(n) << "\"" << endl;
	}

	s << "-------" << endl;

	// Enumerate registered global properties
	c = engine->GetGlobalPropertyCount();
	for( n = 0; n < c; n++ )
	{
		const char *name, *nameSpace;
		int typeId;
		bool isConst;
		const char *group;
		engine->GetGlobalPropertyByIndex(n, &name, &nameSpace, &typeId, &isConst, &group);
		s << "reg prop: ";
		if( isConst ) 
			s << "const ";
		s << engine->GetTypeDeclaration(typeId) << " " << name;
		s << " group: " << (group ? group : "<null>") << endl;
	}

	// Enumerate registered typedefs
	c = engine->GetTypedefCount();
	for( n = 0; n < c; n++ )
	{
		int typeId;
		const char *name = engine->GetTypedefByIndex(n, &typeId);
		s << "reg typedef: " << name << " => " << engine->GetTypeDeclaration(typeId) << endl;
	}

	// Enumerate registered global functions
	c = engine->GetGlobalFunctionCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *func = engine->GetGlobalFunctionByIndex(n);
		const char *group = func->GetConfigGroup();
		s << "reg func: " << func->GetDeclaration() << 
			" group: " << (group ? group : "<null>") << endl;
	}

	// Enumerate registered enums
	c = engine->GetEnumCount();
	for( n = 0; n < c; n++ )
	{
		int eid;
		const char *ename = engine->GetEnumByIndex(n, &eid);

		s << "reg enum: " << ename << endl;

		// List enum values
		for( int e = 0; e < engine->GetEnumValueCount(eid); e++ )
		{
			int value;
			const char *name = engine->GetEnumValueByIndex(eid, e, &value);
			s << " " << name << " = " << value << endl;
		}
	}

	// Enumerate registered func defs
	c = engine->GetFuncdefCount();
	for( n = 0; n < c; n++ )
	{
		asIScriptFunction *funcdef = engine->GetFuncdefByIndex(n);

		s << "reg funcdef: " << funcdef->GetDeclaration() << endl;
	}

	// Get the string factory return type
	int typeId = engine->GetStringFactoryReturnTypeId();
	s << "string factory: " << engine->GetTypeDeclaration(typeId) << endl;

	// Enumerate registered types
	c = engine->GetObjectTypeCount();
	for( n = 0; n < c; n++ )
	{
		DumpObjectType(s, engine->GetObjectTypeByIndex(n));
	}

	//--------------------------------
	// Validate the dump
	if( s.str() != 
		"func: void Test()\n"
		"type: class A : I\n"
		" A@ A()\n"
		" void i(float)\n"
		" void a(int)\n"
		" float f\n"
		"type: class B : A, I\n"
		" B@ B(int)\n"
		" void i(float)\n"
		" void a(int)\n"
		" float f\n"
		"type: interface I\n"
		" void i(float)\n"
		"global: float a\n"
		"global: const float aConst\n"
		"global: E e\n"
		"global: float pi\n"
		"global: I@ i\n"
		"enum: E\n"
		" eval = 0\n"
		" eval2 = 2\n"
		"typedef: real => float\n"
		"import: void ImpFunc() from \"mod\"\n"
		"-------\n"
		"reg prop: float f group: <null>\n"
		"reg prop: const float myConst group: <null>\n"
		"reg typedef: myFloat => float\n"
		"reg func: string formatInt(int64, const string&in, uint arg2 = 0) group: <null>\n"
		"reg func: string formatFloat(double, const string&in, uint arg2 = 0, uint arg3 = 0) group: <null>\n"
		"reg func: int64 parseInt(const string&in, uint arg1 = 10, uint&out arg2 = 0) group: <null>\n"
		"reg func: double parseFloat(const string&in, uint&out arg1 = 0) group: <null>\n"
		"reg func: void func(int&in) group: <null>\n"
		"reg func: void func2() group: test\n"
		"reg enum: myEnum\n"
		" value1 = 1\n"
		" value2 = 2\n"
		"reg funcdef: void Callback(int, int)\n"
		"string factory: string\n"
		"reg type: ref array group: <null>\n"
		" T[]@ _beh_2_(int&in)\n"
		" T[]@ _beh_2_(int&in, uint)\n"
		" T[]@ _beh_2_(int&in, uint, const T&in)\n"
		" beh(4) void _beh_4_()\n"
		" beh(5) void _beh_5_()\n"
		" beh(11) int _beh_11_()\n"
		" beh(12) void _beh_12_()\n"
		" beh(13) bool _beh_13_()\n"
		" beh(14) void _beh_14_(int&in)\n"
		" beh(15) void _beh_15_(int&in)\n"
		" beh(10) bool _beh_10_(int&in, bool&out)\n"
		" beh(3) T[]@ _beh_3_(int&in, uint)\n"
		" T& opIndex(uint)\n"
		" const T& opIndex(uint) const\n"
		" T[]& opAssign(const T[]&in)\n"
		" void insertAt(uint, const T&in)\n"
		" void removeAt(uint)\n"
		" void insertLast(const T&in)\n"
		" void removeLast()\n"
		" uint length() const\n"
		" void reserve(uint)\n"
		" void resize(uint)\n"
		" void sortAsc()\n"
		" void sortAsc(uint, uint)\n"
		" void sortDesc()\n"
		" void sortDesc(uint, uint)\n"
		" void reverse()\n"
		" int find(const T&in) const\n"
		" int find(uint, const T&in) const\n"
		" bool opEquals(const T[]&in) const\n"
		" bool isEmpty() const\n"
		" uint get_length() const\n"
		" void set_length(uint)\n"
		"reg type: val string group: <null>\n"
		" beh(1) void _beh_1_()\n"
		" beh(0) void _beh_0_()\n"
		" beh(0) void _beh_0_(const string&in)\n"
		" string& opAssign(const string&in)\n"
		" string& opAddAssign(const string&in)\n"
		" bool opEquals(const string&in) const\n"
		" int opCmp(const string&in) const\n"
		" string opAdd(const string&in) const\n"
		" uint length() const\n"
		" void resize(uint)\n"
		" uint get_length() const\n"
		" void set_length(uint)\n"
		" bool isEmpty() const\n"
		" uint8& opIndex(uint)\n"
		" const uint8& opIndex(uint) const\n"
		" string& opAssign(double)\n"
		" string& opAddAssign(double)\n"
		" string opAdd(double) const\n"
		" string opAdd_r(double) const\n"
		" string& opAssign(int)\n"
		" string& opAddAssign(int)\n"
		" string opAdd(int) const\n"
		" string opAdd_r(int) const\n"
		" string& opAssign(uint)\n"
		" string& opAddAssign(uint)\n"
		" string opAdd(uint) const\n"
		" string opAdd_r(uint) const\n" 
		" string& opAssign(bool)\n"
		" string& opAddAssign(bool)\n"
		" string opAdd(bool) const\n"
		" string opAdd_r(bool) const\n"
		" string substr(uint arg0 = 0, int arg1 = - 1) const\n"
		" int findFirst(const string&in, uint arg1 = 0) const\n"
		" int findLast(const string&in, int arg1 = - 1) const\n"
		"type: interface MyIntf\n"
		" void func() const\n" )
	{
		cout << s.str() << endl;
		cout << "Failed to get the expected result when dumping the module" << endl << endl;
	}
}


} // namespace

