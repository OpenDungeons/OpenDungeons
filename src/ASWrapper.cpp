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
 * - possible improvements to compilation of scripts (store the state)?
 * - bind all needed classes
 * - find out if executeFunction() is really needed by us (if yes: finish it)
 * - find out if we really need all the asserts (binary size, start up time).
 */

#include <iostream>

#include "angelscript.h"

#include "Console.h"
#include "ResourceManager.h"

#include "ASWrapper.h"

template<> ASWrapper* Ogre::Singleton<ASWrapper>::ms_Singleton = 0;

/*! \brief Initialises AngelScript
 *
 */
ASWrapper::ASWrapper() :
        //create engine
        engine(asCreateScriptEngine(ANGELSCRIPT_VERSION)),
        //create modules
        //TODO: perhaps its better to have more module for different kinds
        //of tasks, like one for AI, one for console, one for gui, etc.
        //Then we'd put them in a map<name, module>
        module(engine->GetModule("asModule", asGM_ALWAYS_CREATE)),
        //create context that runs the script functions
        context(engine->CreateContext())
{
    //register function that gives out standard runtime information
    engine->SetMessageCallback(asMETHOD(ASWrapper, messageCallback), this,
            asCALL_THISCALL);

    //load all .as files from /scripts folder so we can access them
    std::vector<std::string> files = ResourceManager::getSingleton().
            listAllFiles(ResourceManager::getSingleton().getScriptPath());
    for(std::vector<std::string>::iterator i = files.begin(), end = files.end();
            i < end; ++i)
    {
        if(ResourceManager::hasFileEnding(*i, ".as"))
        {
            loadScript(*i);
        }
    }

    //TODO: Can the compiled scripts be saved so that we only need the
    //compilation if something has changed since last start?
    //compile the scripts for faster execution
    module->Build();

    //bind all objects, functions, etc to AngelScript
    registerEverything();
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

/*! \brief executes a script function
 *
 *  \param function The name of the function
 */
void ASWrapper::executeFunction(std::string function)
{
    //tell the engine what function to load
    context->Prepare(engine->GetModule("console")->GetFunctionIdByDecl(
            function.c_str()));

    //TODO: evaluate possible parameters

    //execute the function
    context->Execute();
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
}

/*! \brief Bind all types, functions etc. to the AngelScript engine
 *
 *  Every class and function and variable that should be useable by AngelScript
 *  needs to be registered here. The code is heavily commented.
 */
void ASWrapper::registerEverything()
{
    /* Names of the classes to register. Centrally defined because we need
     * them often, so it will be easier if there's a change in the future.
     * These are the names that AS will use to access the classes. Technically
     * they can have different names than thay have in the C++ code. But we
     * should try to stay as genuine as possible.
     */
    static const char* CONSOLE = "Console";

    /* OVERVIEW of what happens in the following lines
     *
     * Optionally all calls can be followed by an
     *    assert(r >= 0);
     * with int r being the error code of AS. If something goes wrong then the
     * game won't start at all and give the exact code location of the error.
     *
     * for EACH class the registration goes:
     *    engine->RegisterObjectType(name, 0, asOBJ_REF)
     * OR for Singletons (prevents failing instantiation by AS):
     *    engine->RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOHANDLE)
     *
     * Then only for NON-SIngletons we need a constructor, reference counter
     * (telling AS how many references to the object exist) and a dereferencer
     *
     * Constructor (static method in ASWrapper):
     *    engine->RegisterObjectBehaviour(name, asBEHAVE_FACTORY, "name@ f()",
     *        asMETHOD(createInstance<ClassName>), asCALL_CDECL);
     *
     * //TODO: currently we only have a dummy function to get AS working, but
     * maybe, maybe not, we later need reference counting: If we need it:
     * find out how to templatize it
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
     */
}
