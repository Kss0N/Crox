#include "framework_crt.h"
#include "json.h"
#include <stb_ds.h>

#ifdef _DEBUG
#define assert(expr) if(!(expr)) __debugbreak();
#else 
#define assert(expr) 
#endif // _DEBUG


struct Val
{
	JSONtype type;
	JSONvalue v;
};

struct _JSONobject
{
	char* key;
	struct Val value;
};
struct _JSONarray
{
	JSONarray next;
	struct Val value;
};

const char const* IGNORED = " \n\t";

static JSONvalue  copyValue(JSONtype t, JSONvalue v);
static bool copyObjectCallback(_In_ JSONobject src, _In_z_ const char* key, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	JSONobject dst = (JSONobject)userData;
	assert(dst);
	jsonObjectSet(dst, key, t, copyValue(t, v));
	return true;
}
static bool copyArrayCallback(_In_ JSONarray src, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userdata)
{
	JSONarray dst = (JSONarray)userdata;
	if (!dst) return false;
	jsonArrayPushBack(dst, t, copyValue(t, v));
	return true;
}
static JSONvalue copyValue(JSONtype t, JSONvalue v)
{
	switch (t)
	{
	case JSONtype_OBJECT:
	{
		JSONobject obj = jsonCreateObject();
		bool success = obj != NULL && jsonObjectForeach(v.object, copyObjectCallback, obj) != 0;
		if (!success)
		{
			if(obj)
				jsonDestroyObject(obj);
			obj = NULL;
		}
		return JVU(object = obj);
	}
	case JSONtype_ARRAY:
	{
		JSONarray arr = jsonCreateArray();
		bool success = arr != NULL && jsonArrayForeach(v.array, copyArrayCallback, arr) != 0;
		if (!success)
		{
			if (arr)
				jsonDestroyArray(arr);
			arr = NULL;
		}
		return JVU(array = arr);
	}
	case JSONtype_STRING:
	{
		size_t len = strlen(v.string) + 1; //include null terminator
		char* str = malloc(len);
		if(str)
			strcpy_s(str, len, v.string);
		return JVU(string = str);
	}
	default:
		return v;
	}
}



static void destroyValue(JSONtype t, JSONvalue v);
static bool destroyObjectCallback(_In_ JSONobject o, _In_z_ const char* key, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	(o);
	(key);
	(userData);

	destroyValue(t, v);
	return true;
}
static bool destroyArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userdata)
{
	(a);
	(ix);
	(userdata);

	destroyValue(t, v);
	return true;
}
static void destroyValue(JSONtype t, JSONvalue v)
{
	switch (t)
	{
	case JSONtype_OBJECT:
	{
		jsonDestroyObject(v.object);
		break;
	}
	case JSONtype_ARRAY:
	{
		jsonDestroyArray(v.array);
		break;
	}
	case JSONtype_STRING:
	{
		free((void*)v.string);
		break;
	}

	default:
		break; //everything else gets automatically destroyed
	}
}
static void destroyArrayRecursive(JSONarray node, JSONarray last)
{
	if (node->next != last) destroyArrayRecursive(node->next, last);
	free((void*)node);
}



static bool pushFront(_In_ JSONarray a, JSONtype t, JSONvalue v, bool copy)
{
	JSONarray node = malloc(sizeof*node);
	if (!node) return false;

	*node = (struct _JSONarray){ NULL, {t, copy ? copyValue(t,v) : v} };
	if (a->next)
	{
		JSONarray head = a->next->next;
		node->next = head;
		a->next->next = node;
	}
	else
	{
		a->next = node;
		node->next = node;
	}
	a->value.v.uint++;
	return true;
}
static bool pushBack (_In_ JSONarray a, JSONtype t, JSONvalue v, bool copy)
{
	JSONarray node = malloc(sizeof*node);
	if (!node) return false;

	*node = (struct _JSONarray){ NULL, {t, copy ? copyValue(t,v) : v} };
	if (a->next)
	{
		JSONarray head = a->next->next;
		a->next = node;
		node->next = head;
	}
	else
	{
		a->next = node;
		node->next = node;
	}

	a->value.v.uint++;
	return true;
}
static bool setObject(_In_ JSONobject o, _In_z_ const char* key, JSONtype type, JSONvalue value, bool copy)
{
	struct Val v = { type, value };
	shput(o, key, v);
	return true;
}


static JSONobject parseObject(_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** it);
static JSONarray  parseArray (_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** it);
static const char*parseString(_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** it);
static JSONvalue  parseValue (_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** it, JSONtype* type);

static JSONobject parseObject(_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** oIt)
{
	JSONobject obj = jsonCreateObject();
	const char* it = string+1; //skip left '{'
	it += strspn(it, IGNORED);

	while (*it != '}')
	{
		it += strspn(it, IGNORED);
		assert(strchr("\"'", *it) != NULL);

		const char* key = parseString(it, depth, error, &it);
		assert(key);

		it += strspn(it, IGNORED);
		assert(*it == ':');
		it += strspn(++it, IGNORED);

		JSONtype type = JSONtype_UNKNOWN;
		JSONvalue value = parseValue(it, depth, error, &it, &type);
		
		it += strspn(it, IGNORED);
		if (*it == ',') it++;

		bool success = setObject(obj, key, type, value, false);
		assert(success);
	}

	*oIt = it+1; //skip right '}'
	return obj;
}
static JSONarray  parseArray (_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** oIt)
{
	JSONarray array = jsonCreateArray();
	assert(array);

	const char* it = string + 1; //skip left '['
	it += strspn(it, IGNORED);

	while (*it != '}')
	{
		it += strspn(it, IGNORED);

		JSONtype type = JSONtype_UNKNOWN;
		JSONvalue value = parseValue(it, depth, error, &it, &type);

		it += strspn(it, IGNORED);
		if (*it == ',') it++;

		bool success = pushBack(array, type, value, false);
		assert(success);
	}

	*oIt = it + 1; //skip right ']'
	return array;
}
static const char*parseString(_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** oIt)
{
	(depth);
	const size_t CAP_INC_SIZE = 16;

	size_t 
		strCap = 0,
		strLen = 0;
	char* str = NULL;

	const char* it = string + 1; //skip left " or '
	while (strchr("\"'", *it) == NULL)
	{
		if (strLen >= strCap)
		{
			strCap += CAP_INC_SIZE;
			char* nstr = (char*)realloc(str, strCap);
			assert(nstr);
			str = nstr;
		}	

		char c = '\0';
		switch (*it)
		{
		case '%':
		{
			it++;
			if (*it == '%')
			{
				c = *it++;
				break;
			}
			uint64_t val = strtoull(it, &it, 16);
			c = (char)val;
			break;
		}
		case '\\':
		{
			it++;
			switch (*it++)
			{
			case '\\': c = '\\'; break;
			case '\"': c = '\"'; break;
			case '/' : c = '/' ; break;  
			case 'b' : c = '\b'; break;
			case 'f' : c = '\f'; break;
			case 'n' : c = '\n'; break;
			case 't' : c = '\t'; break;
			case 'r' : c = '\r'; break;
			case 'u' :
			{
				uint64_t val = _strtoui64(it, &it, 16);
				char
					hiC = (char)(val >> 8),
					loC = (char)(val >> 0);

				if (strLen >= strCap)
				{
					strCap += CAP_INC_SIZE;
					char* nstr = (char*)realloc(str, strCap);
					assert(nstr);
					str = nstr;
				}
				str[strLen++] = hiC;
				c = loC;
				break;
			}

			default:
				c = '\0';
				break;
			}

			break;
		}
		default:
			c = *it++;
			break;
		}
		str[strLen++] = c;
	}
	
	if (strLen >= strCap)
	{
		strCap += CAP_INC_SIZE;
		char* nstr = (char*)realloc(str, strCap);
		assert(nstr);
		str = nstr;
	}
	str[strLen++] = '\0';

	str = realloc(str, strLen); //resize down
	*oIt = it + 1; // skip right ' or "
	return (const char*)str;
}
static JSONvalue  parseValue (_In_z_ const char* string, _In_ uint8_t depth, char* error, const char** oIt, JSONtype* type)
{
	JSONtype t = JSONtype_UNKNOWN;
	JSONvalue v = {0};

	double number = 0.0;


	if (*string == '{')
	{
		t = JSONtype_OBJECT;
		JSONobject obj = depth != 0 ? parseObject(string, depth - 1, error, oIt) : jsonCreateObject();
		v = JVU(object = obj);
	}
	else if (*string == '[')
	{

		t = JSONtype_ARRAY;
		JSONarray a = depth != 0 ? parseArray(string, depth - 1, error, oIt) : jsonCreateArray();
		v = JVU(array = a);
	}
	else if (strchr("\"'", *string))
	{
		t = JSONtype_STRING;
		const char* str = parseString(string, depth - 1, error, oIt);
		v = JVU(string = str);
	}
	else if (sscanf_s(string, "%lf", &number) != 0)
	{
		if (number == (double)(int64_t)number) //is integer
		{
			t = JSONtype_INTEGER;
			v = JVU(integer = strtoll(string, oIt, 10));
		}
		else
		{
			t = JSONtype_NUMBER;
			v = JVU(number = strtod(string, oIt));
		}
	}
	else if (strncmp(string, "null", strlen("null")) == 0)
	{
		t = JSONtype_NULL;
		v = JVU(string = NULL);
		*oIt = string += strlen("null");
	}
	else if (strncmp(string, "false", strlen("false")) == 0)
	{
		t = JSONtype_BOOLEAN;
		v = JVU(boolean = false);
		*oIt = string += strlen("false");
	}
	else if (strncmp(string, "true", strlen("true")) == 0)
	{
		t = JSONtype_BOOLEAN;
		v = JVU(boolean = true);
		*oIt = string += strlen("true");
	}

	*type = t;
	return v;
}

struct FindCtx
{
	uint32_t ix;
	JSONtype t;
	JSONvalue v;
};
static _Success_(return != false) bool arrayFindValCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	(a);

	struct FindCtx* ctx = (struct FindCtx*)userData;
	assert(ctx);
	if (ctx->ix == ix)
	{
		ctx->t = t;
		ctx->v = v;
		return false; //prematurely break out.
	}
	else return true; 
}

static JSONobject getObject(_In_ JSONobject o, const char* key)
{
	assert(o != NULL);
	return shgetp_null(o, key);
}


//
// -=-=-=- PUBLIC API -=-=-=-
//

uint32_t  jsonArrayGetSize	(_In_ JSONarray a)
{
	return (uint32_t) a->value.v.uint;
}
JSONtype  jsonArrayGetType	(_In_ JSONarray a, _In_ uint32_t ix)
{
	struct FindCtx ctx = { ix };
	if (!jsonArrayForeach(a, arrayFindValCallback, &ctx))
	{
		return ctx.t;
	}
	// else the index does not exist problem
	assert(jsonArrayGetSize(a) > ix);
	return JSONtype_UNKNOWN;
}
JSONvalue jsonArrayGet		(_In_ JSONarray a, _In_ uint32_t ix)
{
	struct FindCtx ctx = {ix};
	if (!jsonArrayForeach(a, arrayFindValCallback, &ctx))
	{
		return ctx.v;
	}
	// else the index does not exist problem
	assert(jsonArrayGetSize(a) > ix);
	return (JSONvalue) { NULL };
}
uint32_t  jsonArrayForeach	(_In_ JSONarray a, _In_ JSONarray_ForeachCallback cb, _Inout_opt_ void* userData)
{
	uint32_t ix = 0;
	const uint32_t s = jsonArrayGetSize(a);
	if (s == 0) return 0;
	
	const JSONarray
		last = a->next,
		first = last->next;
	
	for (JSONarray node = first; ix < s; node = node->next, ix++)
	{
		struct Val v = node->value;
		if (!cb(a, ix, v.type, v.v, userData)) return 0;
	}
	return ix;
}



JSONvalue jsonObjectGet		(_In_ JSONobject o, _In_z_ const char* key)
{
	JSONobject x = getObject(o, key);
	assert(x != NULL);
	return x->value.v;
}
JSONtype  jsonObjectGetType(_In_ JSONobject o, _In_z_ const char* key)
{
	JSONobject x = getObject(o, key);
	return x != NULL ? x->value.type : JSONtype_UNKNOWN;
}
JSONvalue jsonObjectGetOrElse(_In_ JSONobject o, _In_z_ const char* key, JSONvalue _default)
{
	JSONobject x = getObject(o, key);
	return x != NULL ? x->value.v : _default;
}
uint32_t  jsonObjectForeach(_In_ JSONobject o, _In_ JSONobject_ForeachCallback cb, _Inout_opt_ void* userData)
{
	uint32_t i;
	const size_t s = shlenu(o);
	for (i = 0; i < s; i++) if (o[i].key != NULL)
	{
		struct _JSONobject x = o[i];
		if (!cb(o, x.key, x.value.type, x.value.v, userData))
			return 0;
	}
	return i;
}



JSONvalue jsonParseValue(_In_z_ const char* string, _In_ uint8_t maxDepth, _Out_writes_opt_(512) char* error)
{
	const char* it;
	JSONtype type;
	return parseValue(string, maxDepth, error, &it, &type);

}

void jsonDestroyObject(_In_ JSONobject obj)
{
	jsonObjectForeach(obj, destroyObjectCallback, NULL);
	shfree(obj);
}
void jsonDestroyArray(_In_ JSONarray a)
{
	jsonArrayForeach(a, destroyArrayCallback, NULL);

	if (a->next != NULL)
	{
		destroyArrayRecursive(a->next->next, a->next);
		free((void*)a->next);
	}
	free((void*)a);
}

JSONobject jsonCreateObject()
{
	JSONobject obj = NULL;
	sh_new_strdup(obj);
	return obj;
}
JSONarray  jsonCreateArray()
{
	JSONarray a = malloc(sizeof * a);
	if (a)
	{
		a->next = NULL;
		a->value.type = JSONtype_INTEGER;
		a->value.v.uint = 0;
	}
	return a;
}

bool jsonArrayPushBack(_In_ JSONarray a, JSONtype t, JSONvalue v)
{
	return pushBack(a, t, v, true);
}
bool jsonArrayPushFront(_In_ JSONarray a, JSONtype t, JSONvalue v) 
{
	return pushFront(a, t, v, true);
}
bool jsonObjectSet(_In_ JSONobject o, _In_z_ const char* key, JSONtype t, JSONvalue v)
{
	return setObject(o, key, t, v, true);
}