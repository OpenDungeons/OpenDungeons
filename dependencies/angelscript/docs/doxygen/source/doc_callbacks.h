/**

\page doc_callbacks Funcdefs and script callback functions

\ref doc_global_funcdef "Funcdefs" are used to define a function signature for callbacks. This funcdef is then
used to declare variables or function parameters that can hold handles to functions of matching signature.

The application can also \ref asIScriptEngine::RegisterFuncdef "register funcdefs" as part of the application 
interface if the intention is for the script to set callbacks that will be called from the application. Once 
this is done, the application can receive the function handles as an asIScriptFunction pointer which can
then be \ref doc_call_script_func "executed" normally.




*/
