#include "scripthandle.h"
#include <new>
#include <assert.h>
#include <string.h>

BEGIN_AS_NAMESPACE

static void Construct(CScriptHandle *self) { new(self) CScriptHandle(); }
static void Construct(CScriptHandle *self, const CScriptHandle &o) { new(self) CScriptHandle(o); }
static void Construct(CScriptHandle *self, void *ref, int typeId) { new(self) CScriptHandle(ref, typeId); }
static void Destruct(CScriptHandle *self) { self->~CScriptHandle(); }

CScriptHandle::CScriptHandle()
{
	m_ref  = 0;
	m_type = 0;
}

CScriptHandle::CScriptHandle(const CScriptHandle &other)
{
	m_ref  = other.m_ref;
	m_type = other.m_type;

	AddRefHandle();
}

CScriptHandle::CScriptHandle(void *ref, asIObjectType *type)
{
	m_ref  = ref;
	m_type = type;

	AddRefHandle();
}

// This constructor shouldn't be called from the application 
// directly as it requires an active script context
CScriptHandle::CScriptHandle(void *ref, int typeId)
{
	m_ref  = 0;
	m_type = 0;

	Assign(ref, typeId);
}

CScriptHandle::~CScriptHandle()
{
	ReleaseHandle();
}

void CScriptHandle::ReleaseHandle()
{
	if( m_ref && m_type )
	{
		asIScriptEngine *engine = m_type->GetEngine();
		engine->ReleaseScriptObject(m_ref, m_type);

		m_ref  = 0;
		m_type = 0;
	}
}

void CScriptHandle::AddRefHandle()
{
	if( m_ref && m_type )
	{
		asIScriptEngine *engine = m_type->GetEngine();
		engine->AddRefScriptObject(m_ref, m_type);
	}
}

CScriptHandle &CScriptHandle::operator =(const CScriptHandle &other)
{
	// Don't do anything if it is the same reference
	if( m_ref == other.m_ref )
		return *this;

	ReleaseHandle();

	m_ref  = other.m_ref;
	m_type = other.m_type;

	AddRefHandle();

	return *this;
}

void CScriptHandle::Set(void *ref, asIObjectType *type)
{
	if( m_ref == ref ) return;

	ReleaseHandle();

	m_ref  = ref;
	m_type = type;

	AddRefHandle();
}

// This method shouldn't be called from the application 
// directly as it requires an active script context
CScriptHandle &CScriptHandle::Assign(void *ref, int typeId)
{
	// When receiving a null handle we just clear our memory
	if( typeId == 0 )
	{
		Set(0, 0);
		return *this;
	}

	// Dereference received handles to get the object
	if( typeId & asTYPEID_OBJHANDLE )
	{
		// Store the actual reference
		ref = *(void**)ref;
		typeId &= ~asTYPEID_OBJHANDLE;
	}

	// Get the object type
	asIScriptContext *ctx    = asGetActiveContext();
	asIScriptEngine  *engine = ctx->GetEngine();
	asIObjectType    *type   = engine->GetObjectTypeById(typeId);

	Set(ref, type);

	return *this;
}

bool CScriptHandle::operator==(const CScriptHandle &o) const
{
	if( m_ref  == o.m_ref &&
		m_type == o.m_type )
		return true;

	// TODO: If type is not the same, we should attempt to do a dynamic cast,
	//       which may change the pointer for application registered classes

	return false;
}

bool CScriptHandle::operator!=(const CScriptHandle &o) const
{
	return !(*this == o);
}

bool CScriptHandle::Equals(void *ref, int typeId) const
{
	// Null handles are received as reference to a null handle
	if( typeId == 0 )
		ref = 0;

	// Dereference handles to get the object
	if( typeId & asTYPEID_OBJHANDLE )
	{
		// Compare the actual reference
		ref = *(void**)ref;
		typeId &= ~asTYPEID_OBJHANDLE;
	}

	// TODO: If typeId is not the same, we should attempt to do a dynamic cast, 
	//       which may change the pointer for application registered classes

	if( ref == m_ref ) return true;

	return false;
}

// AngelScript: used as '@obj = cast<obj>(ref);'
void CScriptHandle::Cast(void **outRef, int typeId)
{
	// If we hold a null handle, then just return null
	if( m_type == 0 )
	{
		*outRef = 0;
		return;
	}
	
	// It is expected that the outRef is always a handle
	assert( typeId & asTYPEID_OBJHANDLE );

	// Compare the type id of the actual object
	typeId &= ~asTYPEID_OBJHANDLE;
	asIScriptEngine  *engine = m_type->GetEngine();
	asIObjectType    *type   = engine->GetObjectTypeById(typeId);

	*outRef = 0;

	if( type == m_type )
	{
		// Must increase the ref count as we're returning a new reference to the object
		AddRefHandle();
		*outRef = m_ref;
	}
	else if( m_type->GetFlags() & asOBJ_SCRIPT_OBJECT )
	{
		// Attempt a dynamic cast of the stored handle to the requested handle type
		if( engine->IsHandleCompatibleWithObject(m_ref, m_type->GetTypeId(), typeId) )
		{
			// The script type is compatible so we can simply return the same pointer
			AddRefHandle();
			*outRef = m_ref;
		}
	}
	else
	{
		// TODO: Check for the existance of a reference cast behaviour. 
		//       Both implicit and explicit casts may be used
		//       Calling the reference cast behaviour may change the actual 
		//       pointer so the AddRef must be called on the new pointer
	}
}

static void RegisterScriptHandle_Native(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectType("ref", sizeof(CScriptHandle), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_APP_CLASS_CDAK); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Construct, (CScriptHandle *), void), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ref &in)", asFUNCTIONPR(Construct, (CScriptHandle *, const CScriptHandle &), void), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_CONSTRUCT, "void f(const ?&in)", asFUNCTIONPR(Construct, (CScriptHandle *, void *, int), void), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_DESTRUCT, "void f()", asFUNCTIONPR(Destruct, (CScriptHandle *), void), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("ref", asBEHAVE_REF_CAST, "void f(?&out)", asMETHODPR(CScriptHandle, Cast, (void **, int), void), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ref", "ref &opAssign(const ref &in)", asMETHOD(CScriptHandle, operator=), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ref", "ref &opAssign(const ?&in)", asMETHOD(CScriptHandle, Assign), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ref &in) const", asMETHODPR(CScriptHandle, operator==, (const CScriptHandle &) const, bool), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("ref", "bool opEquals(const ?&in) const", asMETHODPR(CScriptHandle, Equals, (void*, int) const, bool), asCALL_THISCALL); assert( r >= 0 );
}

void RegisterScriptHandle(asIScriptEngine *engine)
{
//	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
//		RegisterScriptHandle_Generic(engine);
//	else
		RegisterScriptHandle_Native(engine);
}


END_AS_NAMESPACE
