#include "utils.h"


namespace TestRefCast
{


class typeA
{
public:
	int iRef;

	typeA()
	{
		iRef = 1;
	}

	virtual ~typeA()
	{
	}

	void AddRef()
	{
		iRef++;
	}

	void Release()
	{
		if (--iRef == 0)
		{
			delete this;
		}
	}

};


class typeB : public typeA
{
public:
	int a;
	typeB()
	{
		a = 3;
	}
};


typeA *typeA_Factory()
{
    return new typeA();
}

typeB *typeB_Factory()
{
    return new typeB();
}



typeA* B_to_A(typeB* obj)
{
	if( !obj ) return 0;
	typeA* o = dynamic_cast<typeA*>(obj);

	return o;
}


typeB* A_to_B(typeA* obj)
{
	if( !obj ) return 0;
	typeB* o = dynamic_cast<typeB*>(obj);

	return o;
}



void RegisterA(asIScriptEngine* engine)
{
	int r = 0;
	r = engine->RegisterObjectType("typeA", sizeof(typeA), asOBJ_REF);

	r = engine->RegisterObjectBehaviour("typeA", asBEHAVE_FACTORY, "typeA@ f()", asFUNCTION(typeA_Factory), asCALL_CDECL);
	r = engine->RegisterObjectBehaviour("typeA", asBEHAVE_ADDREF, "void f()", asMETHOD(typeA, AddRef), asCALL_THISCALL);
	r = engine->RegisterObjectBehaviour("typeA", asBEHAVE_RELEASE, "void f()", asMETHOD(typeA, Release), asCALL_THISCALL);
}


void RegisterB(asIScriptEngine* engine)
{
	int r = 0;
	r = engine->RegisterObjectType("typeB", sizeof(typeB), asOBJ_REF);

	r = engine->RegisterObjectBehaviour("typeB", asBEHAVE_FACTORY, "typeB@ f()", asFUNCTION(typeB_Factory), asCALL_CDECL);
	r = engine->RegisterObjectBehaviour("typeB", asBEHAVE_ADDREF, "void f()", asMETHOD(typeB, AddRef), asCALL_THISCALL);
	r = engine->RegisterObjectBehaviour("typeB", asBEHAVE_RELEASE, "void f()", asMETHOD(typeB, Release), asCALL_THISCALL);

	r = engine->RegisterObjectBehaviour("typeB", asBEHAVE_REF_CAST, "typeA@+ f()", asFUNCTION(B_to_A), asCALL_CDECL_OBJLAST);
	r = engine->RegisterObjectBehaviour("typeA", asBEHAVE_REF_CAST, "typeB@+ f()", asFUNCTION(A_to_B), asCALL_CDECL_OBJLAST);
}





static const char* script =
"class CTest\n"
"{\n"
"	typeA@ m_a;\n"
"	typeB@ m_b;\n"

"	CTest()\n"
"	{\n"
"		@m_a = null;\n"
"		@m_b = null;\n"
"	}\n"

"	void dont_work(typeA@ arg)\n"
"	{\n"
"		@m_a = @arg;\n"
"		@m_b = cast<typeB@>(m_a);\n"
"	}\n"

"	void work(typeA@ arg)\n"
"	{\n"
"		typeA@ a = @arg;\n"
"		@m_b = cast<typeB@>(a);\n"
"	}\n"
"};\n";




bool Test()
{
	bool fail = false;
	int r = 0;
	asIScriptEngine* engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);


	RegisterA(engine);
	RegisterB(engine);


	asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
	r = mod->AddScriptSection("test", script, strlen(script));
	r = mod->Build();

	int objType = engine->GetModule("test")->GetTypeIdByDecl("CTest");
	asIScriptObject* testClassObj = (asIScriptObject*)engine->CreateScriptObject(objType);

	typeA* a = new typeB();

	if (testClassObj)
	{
		asIScriptFunction *method = testClassObj->GetObjectType()->GetMethodByName("dont_work");
		asIScriptContext* ctx = engine->CreateContext();

		r = ctx->Prepare(method);
		r = ctx->SetObject(testClassObj);
		r = ctx->SetArgObject(0, a);
		r = ctx->Execute();

		ctx->Release();
		testClassObj->Release();
	}

	a->Release();

	engine->Release();
	return fail;
}



}
