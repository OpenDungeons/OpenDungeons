#include "utils.h"

void PrintException(asIScriptContext *ctx)
{
	asIScriptEngine *engine = ctx->GetEngine();

	int funcID = ctx->GetExceptionFunction();
	asIScriptFunction *func = engine->GetFunctionDescriptorById(funcID);
	printf("mdle : %s\n", func->GetModuleName());
	printf("func : %s\n", func->GetName());
	printf("line : %d\n", ctx->GetExceptionLineNumber());
	printf("desc : %s\n", ctx->GetExceptionString());
}

