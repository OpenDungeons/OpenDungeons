/*!
 * \file   ASWrapper.cpp
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes AngelScript and provides access to its functions
 *
 * AngelScript documentation can be found at:
 * www.angelcode.com/angelscript/sdk/docs/manual/index.html
 */

/* TODO list:
 * - decide how many modules (don't forget to adjust loadScript() then)
 * - find out if addref/release methods are useful or even needed by us
 * - possible improvements to compilation of scripts? (AS has an addon)
 * - bind all needed classes
 * - write a function to pass console input directly to AS
 * - find out if we really need all the asserts (binary size, start up time).
 */

#include <iostream>

#include "angelscript.h"
#include "scriptarray.h"
#include "scripthelper.h"
#include "scriptstdstring.h"

#include "Console.h"
#include "ResourceManager.h"
#include "LogManager.h"

#include "ASWrapper.h"

template<> ASWrapper* Ogre::Singleton<ASWrapper>::ms_Singleton = 0;

/*! \brief Initialises AngelScript
 *
 */
ASWrapper::ASWrapper() :
        //create engine
        engine(asCreateScriptEngine(ANGELSCRIPT_VERSION)),
        //create modules
        module(engine->GetModule("asModule", asGM_ALWAYS_CREATE)),
        //create context that runs the script functions
        context(engine->CreateContext())
{
    //register function that gives out standard runtime information
    engine->SetMessageCallback(asMETHOD(ASWrapper, messageCallback), this,
            asCALL_THISCALL);

    //load all .as files from /scripts folder so we can access them
    const std::string& scriptpath = ResourceManager::getSingleton().getScriptPath();
    std::vector<std::string> files = ResourceManager::getSingleton().
            listAllFiles(scriptpath);
    for(std::vector<std::string>::iterator i = files.begin(), end = files.end();
            i < end; ++i)
    {
        if(ResourceManager::hasFileEnding(*i, ".as"))
        {
            loadScript(scriptpath + "/" + *i);
        }
    }

    //bind all objects, functions, etc to AngelScript
    registerEverything();

    //save the string[] type because it's often used for console interaction
    stringArray = engine->GetObjectTypeById(engine->GetTypeIdByDecl(
    		"string[]"));

    //compile AS code
    module->Build();
}

/*! \brief closes AngelScript
 *
 */
ASWrapper::~ASWrapper()
{
    context->Release();
    engine->Release();
}

/*! \brief loads an AngelScript file and adds it to the module
 *
 *  \param fileName The name of the script file, like "script.as"
 */
void ASWrapper::loadScript(std::string fileName)
{
    std::ifstream scriptFile(fileName.c_str());
    std::string script;

    scriptFile.seekg(0, std::ios::end);
    script.reserve(scriptFile.tellg());
    scriptFile.seekg(0, std::ios::beg);

    script.assign((std::istreambuf_iterator<char>(scriptFile)),
            std::istreambuf_iterator<char>());

    module->AddScriptSection(fileName.c_str(), script.c_str());
}

/*! \brief passes code to the script engine and tries to execute it
 *
 *  \param code The AngelScript code that should be executed
 */
void ASWrapper::executeScriptCode(const std::string& code)
{
    ExecuteString(engine, code.c_str(), module, context);
}

/*! \brief Send AngelScript errors, warnings and information to our console
 *
 * Generates a string in the form of
 *   "ANGELSCRIPT: [section] ([row], [col]) : [type] : [message]"
 * and passes it to our Console
 */
void ASWrapper::messageCallback(const asSMessageInfo* msg, void* param)
{
    const char* type = "";
    switch(msg->type)
    {
        case asMSGTYPE_WARNING:
            type = "WARN";
            break;

        case asMSGTYPE_INFORMATION:
            type = "INFO";
            break;

        default:
            type = "ERR";
            break;
    }

    std::ostringstream output;
    output << "ANGELSCRIPT: " << msg->section << "(" << msg->row << ", "
            << msg->col << ") : " << type << " : " << msg->message;
    Console::getSingleton().print(output.str());
    LogManager::getSingleton().logMessage(output.str());
}

/*! \brief Bind all types, functions etc. to the AngelScript engine
 *
 *  Every class and function and variable that should be useable by AngelScript
 *  needs to be registered here. The code is heavily commented.
 */
void ASWrapper::registerEverything()
{
    /* Register some standard types and features, they are official AS addons
     */
    RegisterStdString(engine);
    RegisterScriptArray(engine, true);

    /* OVERVIEW of what happens in the following lines
     *
     * Optionally all calls can be followed by an
     *    assert(r >= 0);
     * with int r being the error code of AS. If something goes wrong then the
     * game won't start at all and give the exact code location of the error.
     *
     * for EACH class the registration goes:
     *    engine->RegisterObjectType(name, 0, asOBJ_REF);
     * OR for Singletons (prevents failing instantiation by AS):
     *    engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOHANDLE);
     * The singleton reference should then be stored as a global property.
     * We could also register the getSingleton() method, but registering the
     * reference to the object itself makes it easier for script authors
     * because the object already exists globally instead of heaving to get the
     * handle all the time.
     *    engine->RegisterGlobalProperty("Type varname", Type::GetSingleton());
     *
     * Then only for NON-SIngletons we need a constructor, reference counter
     * (telling AS how many references to the object exist) and a dereferencer
     *
     * Constructor (static method in ASWrapper):
     *    engine->RegisterObjectBehaviour(name, asBEHAVE_FACTORY, "name@ f()",
     *        asMETHOD(createInstance<ClassName>), asCALL_CDECL);
     *
     * Reference counter:
     *    engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()",
     *        asFUNCTION(ASWrapper::dummyRef), asCALL_CDECL);
     *
     * Dereferencer (Release Reference):
     *    engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()",
     *        asFUNCTION(ASWrapper::dummyRef), asCALL_CDECL);
     *
     * Now the class is set up and we can add the methods and theoretically
     * also properties. But since we use getters and setters we should never
     * be calling a property directly, just like we do in our C++ code. So if
     * we want to have access to the properties from AS we simply have to
     * register the getters and setters.
     *
     */

    int r = 0;

    //Console with print function
    r = engine->RegisterObjectType("Console", 0, asOBJ_REF | asOBJ_NOHANDLE);
    assert(r >= 0);
    r = engine->RegisterGlobalProperty("Console console",
            Console::getSingletonPtr());
    assert(r >= 0);
    r = engine->RegisterObjectMethod("Console", "void print(string)",
            asMETHOD(Console, print), asCALL_THISCALL);
    assert(r >= 0);
}

/*! \brief Passes the console input to the script that holds all the functions
 *
 *  \param command The vector holding on [0] the command and in [1..n-1] the
 *  arguments
 */
void ASWrapper::executeConsoleCommand(const std::vector<std::string>& command)
{
    CScriptArray* commands = new CScriptArray(command.size(), stringArray);
    for(asUINT i = 0, size = commands->GetSize(); i < size; ++i)
    {
    	*(static_cast<std::string*>(commands->At(i))) = command[i];
    }

    context->Prepare(module->GetFunctionIdByDecl(
    		"void executeConsoleCommand(string[])"));
    context->SetArgObject(0, commands);
    context->Execute();
}
