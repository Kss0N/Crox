#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct _JSONarray* JSONarray;
typedef struct _JSONobject* JSONobject;
typedef union _JSONvalue
{
	int64_t integer;
	uint64_t uint; //if it's known that the value cannot be negative
	bool boolean;
	double number;

	_Null_terminated_ const char* string;
	JSONarray array;
	JSONobject object;
}
JSONvalue;

typedef enum _JSONtype
{
	JSONtype_UNKNOWN = 0,
	JSONtype_INTEGER = 1,
	JSONtype_BOOLEAN = 2,
	JSONtype_NUMBER  = 3,
	JSONtype_STRING  = 4,
	JSONtype_ARRAY	 = 5,
	JSONtype_OBJECT	 = 6,
	JSONtype_NULL	 = 7,
}
JSONtype;

#define JVU(x) (JSONvalue){.x}

typedef _Success_(return != false) bool (*JSONarray_ForeachCallback)(_In_ JSONarray, _In_ uint32_t ix, _In_ JSONtype, _In_ JSONvalue, _Inout_opt_ void* userData);
typedef _Success_(return != false) bool (*JSONobject_ForeachCallback)(_In_ JSONobject, _In_z_ const char* key, _In_ JSONtype, _In_ JSONvalue, _Inout_opt_ void* userData);


uint32_t  jsonArrayGetSize	(_In_ JSONarray);
JSONtype  jsonArrayGetType	(_In_ JSONarray, _In_ uint32_t ix);
JSONvalue jsonArrayGet		(_In_ JSONarray, _In_ uint32_t ix);
uint32_t  jsonArrayForeach	(_In_ JSONarray, _In_ JSONarray_ForeachCallback, _Inout_opt_ void* userData);

JSONvalue jsonObjectGet		 (_In_ JSONobject,_In_z_ const char* key);
JSONtype  jsonObjectGetType	 (_In_ JSONobject,_In_z_ const char* key);
JSONvalue jsonObjectGetOrElse(_In_ JSONobject,_In_z_ const char* key, JSONvalue _default);
uint32_t  jsonObjectForeach	 (_In_ JSONobject,_In_ JSONobject_ForeachCallback, _Inout_opt_ void* userData);


JSONvalue jsonParseValue(_In_z_ const char* string, _In_ uint8_t maxDepth, _Out_writes_opt_(512) char* error);

void jsonDestroyObject(_In_ JSONobject);
void jsonDestroyArray(_In_ JSONarray);

#ifndef JSON_READ_ONLY

JSONobject jsonCreateObject();
JSONarray  jsonCreateArray();

bool jsonArrayPushBack (_In_ JSONarray, JSONtype, JSONvalue);
bool jsonArrayPushFront(_In_ JSONarray, JSONtype, JSONvalue);

bool jsonObjectSet(_In_ JSONobject, _In_z_ const char* key, JSONtype, JSONvalue);


#endif // JSON_READ_ONLY

