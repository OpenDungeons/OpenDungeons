#ifndef UTILS_H
#define UTILS_H

#include <angelscript.h>
#include "../../add_on/scriptstring/scriptstring.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <assert.h>

class COutStream
{
public:
	void Callback(asSMessageInfo *msg) 
	{ 
		const char *msgType = 0;
		if( msg->type == 0 ) msgType = "Error  ";
		if( msg->type == 1 ) msgType = "Warning";
		if( msg->type == 2 ) msgType = "Info   ";

		printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, msgType, msg->message);
	}
};

void PrintException(asIScriptContext *ctx);

#endif

