/**

\page doc_adv_dynamic_build Dynamic compilations

Besides the ordinary \ref doc_compile_script "compilation of scripts" and subsequent 
\ref doc_call_script_func "executions", AngelScript also support dynamically compiling
additional \ref asIScriptModule::CompileFunction "functions" and 
\ref asIScriptModule::CompileGlobalVar "global variables" to incrementally add to the 
scope of a module. These functions and variables will become part of the scope, just
as if they had been included in the initial script compilation which means that subsequent
executions or incremental compilations can use them.

It is also possible to dynamically remove \ref asIScriptModule::RemoveFunction "functions" 
and \ref asIScriptModule::RemoveGlobalVar "variables". Doing so doesn't immediately discard
the functions or variables, so other functions that still refer to them will not fail when
executing them. They will however no longer be visible for new compilations or when searching
for functions or variables in the module.

This kind of dynamic compilations become most useful when dealing with user interaction, 
e.g. an ingame console, or perhaps event handlers, e.g. a trigger on a GUI button click. 

\see \ref doc_addon_helpers "ExecuteString() add-on", \ref doc_samples_console "Console sample"




*/
