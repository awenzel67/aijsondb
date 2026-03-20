#ifndef DOMINODBINDEX_H
#define DOMINODBINDEX_H
#include "quickjs.h"
#include "quickjs-libc.h"
int aijsondb_index(JSContext* ctx);
int aijsondb_execute_script(JSContext* ctx, const char* script);
void js_error_message(JSContext* ctx, JSValue val, char* buffer, int nbuffer);
#define UNKOWN_JS_ERROR "Unknown JavaScript error"
#endif