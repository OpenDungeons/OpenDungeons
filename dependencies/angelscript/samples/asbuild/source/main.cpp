#include <iostream>  // cout
#include <assert.h>  // assert()
#include <string.h>  // strstr()
#include <angelscript.h>
#include "../../../add_on/scriptbuilder/scriptbuilder.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#include <direct.h>
#endif
#ifdef _WIN32_WCE
#include <windows.h> // For GetModuleFileName
#endif

using namespace std;

// Function prototypes
int ConfigureEngine(asIScriptEngine *engine, const char *configFile);
int CompileScript(asIScriptEngine *engine, const char *scriptFile);
int SaveBytecode(asIScriptEngine *engine, const char *outputFile);
static const char *GetCurrentDir(char *buf, size_t size);
asETokenClass GetToken(asIScriptEngine *engine, string &token, const string &text, asUINT &pos);
asUINT GetLineNumber(const string &text, asUINT pos);

void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

int main(int argc, char **argv)
{
	int r;

	if( argc < 4 )
	{
		cout << "Usage: " << endl;
		cout << "asbuild <config file> <script file> <output>" << endl;
		cout << " <config file>  is the file with the application interface" << endl;
		cout << " <script file>  is the script file that should be compiled" << endl;
		cout << " <output>       is the name that the compiled script will be saved as" << endl;
		return -1;
	}

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// The script compiler will send any compiler messages to the callback
	engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL);

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	r = ConfigureEngine(engine, argv[1]);
	if( r < 0 ) return -1;
	
	// Compile the script code
	r = CompileScript(engine, argv[2]);
	if( r < 0 ) return -1;

	// Save the bytecode
	r = SaveBytecode(engine, argv[3]);
	if( r < 0 ) return -1;

	// Release the engine
	engine->Release();

	return 0;
}

// This function will register the application interface, 
// based on information read from a configuration file. 
int ConfigureEngine(asIScriptEngine *engine, const char *configFile)
{
	int r;

	// Since we are only going to compile the script and never actually execute it,
	// we turn off the initialization of global variables, so that the compiler can
	// just register dummy types and functions for the application interface.
	r = engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false); assert( r >= 0 );

	// Open the config file
#if _MSC_VER >= 1500
	FILE *f = 0;
	fopen_s(&f, configFile, "rb");
#else
	FILE *f = fopen(configFile, "rb");
#endif
	if( f == 0 )
	{
		// Write a message to the engine's message callback
		char buf[256];
		string msg = "Failed to open config file in path: '" + string(GetCurrentDir(buf, 256)) + "'";
		engine->WriteMessage(configFile, 0, 0, asMSGTYPE_ERROR, msg.c_str());
		return -1;
	}
	
	// Determine size of the file
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);

	// On Win32 it is possible to do the following instead
	// int len = _filelength(_fileno(f));

	// Read the entire file
	string config;
	config.resize(len);
	size_t c = fread(&config[0], len, 1, f);

	fclose(f);

	if( c == 0 ) 
	{
		// Write a message to the engine's message callback
		char buf[256];
		string msg = "Failed to load config file in path: '" + string(GetCurrentDir(buf, 256)) + "'";
		engine->WriteMessage(configFile, 0, 0, asMSGTYPE_ERROR, msg.c_str());
		return -1;
	}	

	// Process the configuration file and register each entity
	asUINT pos  = 0; 
	while( pos < config.length() )
	{
		string token;
		// TODO: The position where the initial token is found should be stored for error messages
		GetToken(engine, token, config, pos);
		if( token == "namespace" )
		{
			string ns;
			GetToken(engine, ns, config, pos);

			r = engine->SetDefaultNamespace(ns.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to set namespace");
				return -1;
			}
		}
		else if( token == "access" )
		{
			string maskStr;
			GetToken(engine, maskStr, config, pos);
			asDWORD mask = strtol(maskStr.c_str(), 0, 16);
			engine->SetDefaultAccessMask(mask);
		}
		else if( token == "objtype" )
		{
			string name, flags;
			GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			GetToken(engine, flags, config, pos);

			// The size of the value type doesn't matter, because the 
			// engine must adjust it anyway for different platforms
			r = engine->RegisterObjectType(name.c_str(), (atol(flags.c_str()) & asOBJ_VALUE) ? 1 : 0, atol(flags.c_str()));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object type");
				return -1;
			}
		}
		else if( token == "objbeh" )
		{
			string name, behaviour, decl;
			GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			GetToken(engine, behaviour, config, pos);
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			asEBehaviours behave = static_cast<asEBehaviours>(atol(behaviour.c_str()));
			if( behave == asBEHAVE_TEMPLATE_CALLBACK )
			{
				// TODO: How can we let the compiler register this? Maybe through a plug-in system? Or maybe by implementing the callback as a script itself
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_WARNING, "Cannot register template callback without the actual implementation");
			}
			else
			{
				r = engine->RegisterObjectBehaviour(name.c_str(), behave, decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
				if( r < 0 )
				{
					engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register behaviour");
					return -1;
				}
			}
		}
		else if( token == "objmthd" )
		{
			string name, decl;
			GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterObjectMethod(name.c_str(), decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object method");
				return -1;
			}
		}
		else if( token == "objprop" )
		{
			string name, decl;
			GetToken(engine, name, config, pos);
			name = name.substr(1, name.length() - 2);
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			asIObjectType *type = engine->GetObjectTypeById(engine->GetTypeIdByDecl(name.c_str()));
			if( type == 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Type doesn't exist for property registration");
				return -1;
			}

			// All properties must have different offsets in order to make them 
			// distinct, so we simply register them with an incremental offset
			r = engine->RegisterObjectProperty(name.c_str(), decl.c_str(), type->GetPropertyCount());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register object property");
				return -1;
			}
		}
		else if( token == "intf" )
		{
			string name, size, flags;
			GetToken(engine, name, config, pos);

			r = engine->RegisterInterface(name.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register interface");
				return -1;
			}
		}
		else if( token == "intfmthd" )
		{
			string name, decl;
			GetToken(engine, name, config, pos);
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterInterfaceMethod(name.c_str(), decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register interface method");
				return -1;
			}
		}
		else if( token == "func" )
		{
			string decl;
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterGlobalFunction(decl.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register global function");
				return -1;
			}
		}
		else if( token == "prop" )
		{
			string decl;
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			// All properties must have different offsets in order to make them 
			// distinct, so we simply register them with an incremental offset.
			// The pointer must also be non-null so we add 1 to have a value.
			r = engine->RegisterGlobalProperty(decl.c_str(), (void*)(engine->GetGlobalPropertyCount()+1));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register global property");
				return -1;
			}
		}
		else if( token == "strfactory" )
		{
			string type;
			GetToken(engine, type, config, pos);
			type = type.substr(1, type.length() - 2);

			r = engine->RegisterStringFactory(type.c_str(), asFUNCTION(0), asCALL_GENERIC);
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register string factory");
				return -1;
			}
		}
		else if( token == "defarray" )
		{
			string type;
			GetToken(engine, type, config, pos);
			type = type.substr(1, type.length() - 2);

			r = engine->RegisterDefaultArrayType(type.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register the default array type");
				return -1;
			}
		}
		else if( token == "enum" )
		{
			string type;
			GetToken(engine, type, config, pos);
			
			r = engine->RegisterEnum(type.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register enum type");
				return -1;
			}
		}
		else if( token == "enumval" )
		{
			string type, name, value;
			GetToken(engine, type, config, pos);
			GetToken(engine, name, config, pos);
			GetToken(engine, value, config, pos);

			r = engine->RegisterEnumValue(type.c_str(), name.c_str(), atol(value.c_str()));
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register enum value");
				return -1;
			}
		}
		else if( token == "typedef" )
		{
			string type, decl;
			GetToken(engine, type, config, pos);
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterTypedef(type.c_str(), decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register typedef");
				return -1;
			}
		}
		else if( token == "funcdef" )
		{
			string decl;
			GetToken(engine, decl, config, pos);
			decl = decl.substr(1, decl.length() - 2);

			r = engine->RegisterFuncdef(decl.c_str());
			if( r < 0 )
			{
				engine->WriteMessage(configFile, GetLineNumber(config, pos), 0, asMSGTYPE_ERROR, "Failed to register funcdef");
				return -1;
			}
		}
	}

	engine->WriteMessage(configFile, 0, 0, asMSGTYPE_INFORMATION, "Configuration successfully registered");
	
	return 0;
}

asETokenClass GetToken(asIScriptEngine *engine, string &token, const string &text, asUINT &pos)
{
	int len;
	asETokenClass t = engine->ParseToken(&text[pos], text.length() - pos, &len);
	while( (t == asTC_WHITESPACE || t == asTC_COMMENT) && pos < text.length() )
	{
		pos += len;
		t = engine->ParseToken(&text[pos], text.length() - pos, &len);
	}

	token.assign(&text[pos], len);

	pos += len;

	return t;
}

asUINT GetLineNumber(const string &text, asUINT pos)
{
	asUINT count = 1;
	for( asUINT n = 0; n < pos; n++ )
		if( text[n] == '\n' )
			count++;

	return count;
}

int CompileScript(asIScriptEngine *engine, const char *scriptFile)
{
	int r;

	CScriptBuilder builder;
	r = builder.StartNewModule(engine, "build");
	if( r < 0 ) return -1;

	r = builder.AddSectionFromFile(scriptFile);
	if( r < 0 ) return -1;

	r = builder.BuildModule();
	if( r < 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Script failed to build");
		return -1;
	}

	engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_INFORMATION, "Script successfully built");

	return 0;
}

class CBytecodeStream : public asIBinaryStream
{
public:
	CBytecodeStream() {f = 0;}
	~CBytecodeStream() { if( f ) fclose(f); }

	int Open(const char *filename)
	{
		if( f ) return -1;
#if _MSC_VER >= 1500
		fopen_s(&f, filename, "wb");
#else
		f = fopen(filename, "wb");
#endif
		if( f == 0 ) return -1;
		return 0;
	}
	void Write(const void *ptr, asUINT size) 
	{
		if( size == 0 || f == 0 ) return; 
		fwrite(ptr, size, 1, f); 
	}
	void Read(void *, asUINT ) {}

protected:
	FILE *f;
};

int SaveBytecode(asIScriptEngine *engine, const char *outputFile)
{
	CBytecodeStream stream;
	int r = stream.Open(outputFile);
	if( r < 0 )
	{
		engine->WriteMessage(outputFile, 0, 0, asMSGTYPE_ERROR, "Failed to open output file for writing");
		return -1;
	}

	asIScriptModule *mod = engine->GetModule("build");
	if( mod == 0 )
	{
		engine->WriteMessage(outputFile, 0, 0, asMSGTYPE_ERROR, "Failed to retrieve the compiled bytecode");
		return -1;
	}

	r = mod->SaveByteCode(&stream);
	if( r < 0 )
	{
		engine->WriteMessage(outputFile, 0, 0, asMSGTYPE_ERROR, "Failed to write the bytecode");
		return -1;
	}

	engine->WriteMessage(outputFile, 0, 0, asMSGTYPE_INFORMATION, "Bytecode successfully saved");

	return 0;
}

static const char *GetCurrentDir(char *buf, size_t size)
{
#ifdef _MSC_VER
#ifdef _WIN32_WCE
    static TCHAR apppath[MAX_PATH] = TEXT("");
    if (!apppath[0])
    {
        GetModuleFileName(NULL, apppath, MAX_PATH);

        
        int appLen = _tcslen(apppath);

        // Look for the last backslash in the path, which would be the end
        // of the path itself and the start of the filename.  We only want
        // the path part of the exe's full-path filename
        // Safety is that we make sure not to walk off the front of the 
        // array (in case the path is nothing more than a filename)
        while (appLen > 1)
        {
            if (apppath[appLen-1] == TEXT('\\'))
                break;
            appLen--;
        }

        // Terminate the string after the trailing backslash
        apppath[appLen] = TEXT('\0');
    }
#ifdef _UNICODE
    wcstombs(buf, apppath, min(size, wcslen(apppath)*sizeof(wchar_t)));
#else
    memcpy(buf, apppath, min(size, strlen(apppath)));
#endif

    return buf;
#else
	return _getcwd(buf, (int)size);
#endif
#elif defined(__APPLE__)
	return getcwd(buf, size);
#else
	return "";
#endif
}


