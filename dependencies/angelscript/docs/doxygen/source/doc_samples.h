/**

\page doc_samples Samples


This page gives a brief description of the samples that you'll find in the /sdk/samples/ folder.

 - \subpage doc_samples_tutorial
 - \subpage doc_samples_concurrent
 - \subpage doc_samples_console
 - \subpage doc_samples_corout
 - \subpage doc_samples_events
 - \subpage doc_samples_incl
 - \subpage doc_samples_asbuild
 - \subpage doc_samples_asrun
 - \subpage doc_samples_game



\page doc_samples_tutorial Tutorial

<b>Path:</b> /sdk/samples/tutorial/

This sample was written with the intention of explaining the basics of 
AngelScript, that is, how to configure the engine, load and compile a script, 
and finally execute a script function with parameters and return value.


 - \ref asIScriptEngine::SetMessageCallback
 - LineCallback() function which aborts execution when the time is up
 - Strings
 - Registered global functions
 - Script function parameters and return value
 - Retrieving information about script exceptions
 - asIScriptGeneric for when the library doesn't support native calling convention






\page doc_samples_concurrent Concurrent scripts

<b>Path:</b> /sdk/samples/concurrent/

This sample shows how to execute two or more long running scripts
\ref doc_adv_concurrent "concurrently". The scripts voluntarily hand over the control to the next script in 
the queue by calling the function sleep().


 - \ref doc_addon_ctxmgr
 - Multiple scripts running in parallel
 - sleep()
 - Strings
 - Registered global functions




\page doc_samples_console Console

<b>Path:</b> /sdk/samples/console/

This sample implements a simple interactive console, which lets the user type in 
commands and also evaluate simple script statements to manipulate the application.

The user is also able to define new variables and functions from the command line. 
These functions can then be executed to perform automated tasks. 

 - \ref doc_addon_helpers "ExecuteString"
 - \ref asIScriptModule::CompileFunction "CompileFunction", \ref asIScriptModule::CompileGlobalVar "CompileGlobalVar", \ref asIScriptModule::RemoveFunction "RemoveFunction", \ref asIScriptModule::RemoveGlobalVar "RemoveGlobalVar"
 - Enumerate global functions and variables
 - \ref doc_addon_std_string "Strings"
 - Registered global functions and properties
 - Special function _grab() with overloads to receive and print resulting value from script statements



\page doc_samples_corout Co-routines

<b>Path:</b> /sdk/samples/coroutine/

This sample shows how \ref doc_adv_coroutine "co-routines" can be implemented with AngelScript. Co-routines are
threads that can be created from the scripts, and that work together by voluntarily passing control
to each other by calling yield().

 - \ref doc_addon_ctxmgr
 - Co-routines created from the scripts with variable parameter structure.
 - Strings
 - Registered global functions
 - Handling the variable argument type
 - Passing arguments to script functions


\page doc_samples_events Events

<b>Path:</b> /sdk/samples/events/

This sample has the script engine execute a long running script. The script execution is regularly 
interrupted by the application so that keyboard events can be processed, which execute another short 
script before resuming the execution of the main script. The event handling scripts change the state 
of the long running script.

 - LineCallback() function which suspends execution when the time is up
 - Strings
 - Registered global functions
 - Scripted event handlers



\page doc_samples_incl Include directive

<b>Path:</b> /sdk/samples/include/

This sample shows how to implement a very simple preprocessor to add support for the \#include 
directive, which allow the script writer to reuse common script code. The preprocessor simply adds 
the included scripts as multiple script sections, which is ok as AngelScript is able to resolve global 
declarations independently of their order. The preprocessor also makes sure that a script file is only 
included once, so the script writer doesn't have to take extra care to avoid multiple includes or even 
complicated circular includes.

 - \ref doc_addon_build
 - LineCallback() functions which aborts execution when the time is up
 - Processing the \#include directive
 - Circular \#includes are resolved automatically




\page doc_samples_asbuild Generic compiler

<b>Path:</b> /sdk/samples/asbuild/

This sample shows how a generic bytecode compiler can be built to provide offline compilation of scripts.
It takes as input a configuration file that defines the application interface, registers it using dummy
functions and properties, then it compiles the script and saves the bytecode to a file on disk.

The configuration file should preferably be created automatically by calling the \ref doc_addon_helpers "WriteConfigToFile" 
helper function. This function should be called from the application that will execute the bytecode, after 
the application interface has been fully registered.

 - \ref doc_adv_precompile 
 - \ref doc_addon_build
 - \ref asIScriptEngine::SetMessageCallback
 - \ref asEP_INIT_GLOBAL_VARS_AFTER_BUILD
 - \ref asIScriptEngine::ParseToken
 - \ref asIScriptModule::SaveByteCode
 - \ref asIBinaryStream





\page doc_samples_asrun Commandline runner

<b>Path:</b> /sdk/samples/asrun/

This samples gives a very basic commandline executer for AngelScripts. It currently doesn't
allow the user to do very much as it is just a sample, but serves as a good foundation for
building a useful commandline tool.

It also implements a fully functional commandline debugger, with support for setting 
breakpoints, stepping through the code, examining variables, etc.

 - \ref doc_debug
 - \ref doc_addon_debugger
 - \ref doc_addon_std_string
 - \ref doc_addon_array
 - \ref doc_addon_build
 - Passing commandline arguments to script




 \page doc_samples_game Game
 
 <b>Path:</b> /sdk/samples/game/
 
 This sample shows one way of integrating the scripting library in a game engine. It is a 
 simple game where the player is trying to avoid getting eaten by zombies. Each game object type
 has it's own script that controls its behaviour, these are loaded independently into separately
 modules. 
 
 - \ref asIScriptModule
 - \ref asIObjectType::SetUserData
 - \ref doc_global_interface
 - \ref doc_script_class
 - \ref doc_addon_build
 - \ref doc_addon_handle
 - \ref doc_script_shared


*/
