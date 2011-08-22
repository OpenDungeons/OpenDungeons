/*!
 * \file   ASWrapper.cpp
 * \date   08 July 2011
 * \author StefanP.MUC
 * \brief  Initializes AngelScript and provides access to its functions
 *
 * AngelScript documentation can be found at:
 *  www.angelcode.com/angelscript/sdk/docs/manual/index.html
 * The Ogre-Angelscript binding project is something to keep an eye on:
 *  code.google.com/p/ogre-angelscript/
 */

/* TODO list:
 * - decide how many modules (don't forget to adjust loadScript() then)
 * - find out if addref/release methods are useful or even needed by us
 * - possible improvements to compilation of scripts? (AS has an addon)
 * - bind all needed classes
 * - find out if we really need all the asserts (binary size, start up time).
 */

#include <iostream>
#include <sstream>
#include <string>

#include "angelscript.h"
#include "scriptarray.h"
#include "scripthelper.h"
#include "scriptstdstring.h"

#include "CameraManager.h"
#include "Console.h"
#include "LogManager.h"
#include "ODApplication.h"
#include "ODFrameListener.h"
#include "ResourceManager.h"

#include "ASWrapper.h"

template<> ASWrapper* Ogre::Singleton<ASWrapper>::ms_Singleton = 0;

/*! \brief Initialises AngelScript
 *
 */
ASWrapper::ASWrapper() :
        engine  (asCreateScriptEngine(ANGELSCRIPT_VERSION)),
        module  (engine->GetModule("asModule", asGM_ALWAYS_CREATE)),
        context (engine->CreateContext())
{
    LogManager::getSingleton().logMessage(
            "*** Initialising script engine AngelScript ***");

    //register function that gives out standard runtime information
    engine->SetMessageCallback(asMETHOD(ASWrapper, messageCallback), this,
            asCALL_THISCALL);

    //load all .as files from /scripts folder so we can access them
    const std::string& scriptpath = ResourceManager::getSingleton().
            getScriptPath();
    std::vector<std::string> files = ResourceManager::getSingleton().
            listAllFiles(scriptpath);
    for(std::vector<std::string>::iterator i = files.begin(),
            end = files.end(); i < end; ++i)
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

    //Compile AS code, syntax errors will be printed to our Console
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
 *   AS: [section] ([row], [col]) : [type] :
 *     [message]
 * and passes it to our Console and the LogManager
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
    output << "AS: " << msg->section << "(" << msg->row << ", "
            << msg->col << ") : " << type << " : \n  " << msg->message;
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

    //return value of engine for assert check
    int r = 0;

    //helper functions
    r = engine->RegisterGlobalFunction(
            "int stringToInt(string &in)",
            asFunctionPtr(ASWrapper::stringToInt),
            asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction(
            "double stringToFloat(string &in)",
            asFunctionPtr(ASWrapper::stringToFloat),
            asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction(
            "bool checkIfInt(string &in)",
            asFunctionPtr(ASWrapper::checkIfInt),
            asCALL_CDECL); assert(r >= 0);
    r = engine->RegisterGlobalFunction(
            "bool checkIfFloat(string &in)",
            asFunctionPtr(ASWrapper::checkIfFloat),
            asCALL_CDECL); assert(r >= 0);

    //some variabless
    r = engine->RegisterGlobalProperty(
            "double MAXFPS",
            &ODApplication::MAX_FRAMES_PER_SECOND); assert(r >= 0);

    //Console
    r = engine->RegisterObjectType(
            "Console", 0, asOBJ_REF | asOBJ_NOHANDLE); assert(r >= 0);
    r = engine->RegisterGlobalProperty(
            "Console console",
            Console::getSingletonPtr()); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "Console",
            "void print(string)",
            asMETHOD(Console, print),
            asCALL_THISCALL); assert(r >= 0);

    //LogManager
    r = engine->RegisterObjectType(
            "LogManager", 0, asOBJ_REF | asOBJ_NOHANDLE); assert(r >= 0);
    r = engine->RegisterGlobalProperty(
            "LogManager logManager",
            LogManager::getSingletonPtr()); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "LogManager",
            "void logMessage(string)",
            asMETHOD(LogManager, logMessage),
            asCALL_THISCALL); assert(r >= 0);

    //ODFrameListener
    r = engine->RegisterObjectType(
            "ODFrameListener", 0, asOBJ_REF | asOBJ_NOHANDLE); assert(r >= 0);
    r = engine->RegisterGlobalProperty(
            "ODFrameListener frameListener",
            ODFrameListener::getSingletonPtr()); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "ODFrameListener",
            "void requestExit()",
            asMETHOD(ODFrameListener, requestExit),
            asCALL_THISCALL); assert(r >= 0);

    //CameraManager
    r = engine->RegisterObjectType(
            "CameraManager", 0, asOBJ_REF | asOBJ_NOHANDLE); assert(r >= 0);
    r = engine->RegisterGlobalProperty(
            "CameraManager cameraManager",
            CameraManager::getSingletonPtr()); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "CameraManager",
            "void setMoveSpeedAccel(float &in)",
            asMETHOD(CameraManager, setMoveSpeedAccel),
            asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "CameraManager",
            "float& getMoveSpeed()",
            asMETHOD(CameraManager, getMoveSpeed),
            asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "CameraManager",
            "void setRotateSpeed(float &in)",
            asMETHOD(CameraManager, setRotateSpeed),
            asCALL_THISCALL); assert(r >= 0);
    r = engine->RegisterObjectMethod(
            "CameraManager",
            "float getRotateSpeed()",
            asMETHOD(CameraManager, getRotateSpeed),
            asCALL_THISCALL); assert(r >= 0);
}

/*! \brief Passes the console input to the script that holds all the functions
 *
 *  \param command The vector holding on [0] the command and in [1..n-1] the
 *                 arguments
 */
void ASWrapper::executeConsoleCommand(
        const std::vector<std::string>& fullCommand)
{
    CScriptArray* arguments = new CScriptArray(fullCommand.size() - 1,
            stringArray);
    for(asUINT i = 0, size = arguments->GetSize(); i < size; ++i)
    {
    	*(static_cast<std::string*>(arguments->At(i))) = fullCommand[i + 1];
    }

    context->Prepare(module->GetFunctionIdByDecl(
    		"void executeConsoleCommand(string &in, string[] &in)"));
    context->SetArgAddress(0, const_cast<std::string*>(&fullCommand[0]));
    context->SetArgObject(1, arguments);
    context->Execute();
    delete arguments;
}

/*! \brief Script helper function, converts a string to an int
 *
 *  \param str The string to be converted
 *  \return The converted number
 */
int ASWrapper::stringToInt(const std::string& str)
{
    std::stringstream stream(str);
    int i = 0;
    stream >> i;
    return i;
}

/*! \brief Script helper function, converts a string to a float
 *
 *  \param str The string to be converted
 *  \return The converted number
 */
float ASWrapper::stringToFloat(const std::string& str)
{
    std::stringstream stream(str);
    float f = 0;
    stream >> f;
    return f;
}

/*! \brief Script helper function, checks if a string contains an int
 *
 *  \param str The string to be checked
 *  \return true if string contains an int, else false
 */
bool ASWrapper::checkIfInt(const std::string& str)
{
    std::istringstream stream(str);
    int a;
    return (stream >> a);
}

/*! \brief Script helper function, checks if a string contains a float
 *
 *  \param str The string to be checked
 *  \return true if string contains a float, else false
 */
bool ASWrapper::checkIfFloat(const std::string& str)
{
    std::istringstream stream(str);
    float f;
    return (stream >> f);
}
