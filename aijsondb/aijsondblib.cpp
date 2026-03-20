#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <string>
#include <map>
#include <vector>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <fstream>
#include "aijsondblib.h"
#include "quickjs.h"
#include "quickjs-libc.h"
#include "aijsondbindex.h"

static JSContext* JS_NewCustomContext(JSRuntime* rt)
{
	JSContext* ctx = JS_NewContext(rt);
	if (!ctx)
		return NULL;
	return ctx;
}

std::mutex mtx;
static std::map<std::string, std::vector<std::string>> jobject_cache;
static std::string jobject_cache_schema;
static std::string last_error_message;


/*
char* read_file_to_string(const char* filename) {
	FILE* f = fopen(filename, "rb");
	if (!f) {
		perror("Error opening file");
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long length = ftell(f);
	rewind(f);

	const char* prefix = "let data=";
	domino_data_len = length  + 1+ 9;
	char* buffer = (char*) malloc(domino_data_len+domino_data_query_len_max);

	if (!buffer) {
		perror("Memory allocation failed");
		fclose(f);
		return NULL;
	}
	
	for (int i = 0; prefix[i] != '\0'; i++) {
		buffer[i] = prefix[i];
	}

	size_t read_len = fread(buffer+9, 1, length, f);
	buffer[read_len+9] = '\0'; // Null-terminate the string

	fclose(f);
	return buffer;
}
*/

int aijsondb_free_data()
{
	std::lock_guard<std::mutex> lock(mtx);
	jobject_cache.clear();
	last_error_message.clear();
	return 0;
}

const char* aijsondb_last_error()
{
	std::lock_guard<std::mutex> lock(mtx);
	return last_error_message.c_str();
}
int load_cache_and_validate(const char* filepath);
int aijsondb_load_data(const char* filepath_data,const char* filepath_schema)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		jobject_cache_schema.clear();
		std::ifstream file(filepath_schema, std::ios::in | std::ios::binary);
		if (!file) {
		    last_error_message=	"Error opening schema file";
			return -1;
		}
		std::ostringstream ss;
		ss << file.rdbuf(); // Efficiently copies the entire buffer
		jobject_cache_schema=ss.str();
	}
	return load_cache_and_validate(filepath_data);
	//printf("Data loaded successfully:\n%s\n", domino_data);
}


int init_functions(JSContext* ctx);

bool is_virtual_result_object(JSContext* ctx, const JSValue& jsv)
{
	bool is_virtual = false;
	if (!JS_IsObject(jsv))
		return false;

	int64_t length;
	int gu = JS_GetLength(ctx, jsv, &length);
	if (length > 0)
	{
		return false;
	}

	JSValue jsvo = JS_GetPropertyStr(ctx, jsv, "eindex");
	if (JS_IsException(jsvo))
		return false;

	if (!JS_IsUndefined(jsvo))
	{
		is_virtual = true;		
	}
	JS_FreeValue(ctx, jsvo);
	return is_virtual;
}

bool is_virtual_result_array(JSContext* ctx, const JSValue& jsv)
{
	bool is_virtual = false;
	if (!JS_IsObject(jsv))
		return false;

	int64_t length;
	int gu = JS_GetLength(ctx, jsv, &length);
	if (length == 0)
	{
		return false;
	}

	is_virtual = true;
	for (int i=0;i<length;i++)
	{
		JSValue jsve = JS_GetPropertyUint32(ctx, jsv, i);
		if (!JS_IsException(jsve))
		{
			if(!is_virtual_result_object(ctx, jsve))
			{
				is_virtual=false;
				JS_FreeValue(ctx, jsve);
				break;
			}
			JS_FreeValue(ctx, jsve);
		}
	}
	return is_virtual;
}


bool handle_virtual_object(const JSValue& jsv, JSContext* ctx, char* buffer, int nbuffer)
{
	if (!JS_IsObject(jsv))
		return false;

	int64_t length;
	int gu = JS_GetLength(ctx, jsv, &length);

	if (length > 0)
		return false;

	JSValue jsvo = JS_GetPropertyStr(ctx, jsv, "eindex");
	if (JS_IsException(jsvo))
		return false;

	if (JS_IsUndefined(jsvo))
	{
		JS_FreeValue(ctx, jsvo);
		return false;
	}
	
	int32_t eindex;
	if (JS_ToInt32(ctx, &eindex, jsvo) < 0)
		return false;


	if (eindex < 0)
		return false;

	if (eindex>=jobject_cache["employees"].size())
		return false;

	
					//printf("eindex==%d\n", eindex);
	std::string jvalue = jobject_cache["employees"][eindex];
	buffer[0] = '\0';
	bool ret=false;
	if (jvalue.size() < nbuffer - 1) {
		strcpy(buffer, jvalue.c_str());
		ret = true;
	}
	else
	{
	}
	//JS_FreeValue(ctx, jsv);
	return ret;
}

bool add_to_buffer(char* buffer, int nbuffer, const char* textf, int& irun)
{
	if (irun < 0)
		return false;
	if (irun + 1 >= nbuffer)
		return false;

	if (buffer == NULL)
		return false;
	if (irun + strlen(textf) + 1 >= nbuffer)
		return false;
	strcpy(buffer + irun, textf);
	irun += strlen(textf);
	return true;
}

bool handle_virtual_array(const JSValue& jsv, JSContext* ctx, char* buffer, int nbuffer)
{
	if (!JS_IsObject(jsv))
		return false;

	int64_t length;
	int gu = JS_GetLength(ctx, jsv, &length);

	if (length <= 0)
		return false;

	buffer[0] = '\0';
	int ir = 0;
	bool added=add_to_buffer(buffer, nbuffer, "[", ir);
	if (!added)
		return false;

	for (int i = 0; i < length; i++)
	{
		JSValue jsve = JS_GetPropertyUint32(ctx, jsv, i);
		if (!JS_IsException(jsve))
		{
			char* buffer_loc = new char[nbuffer];
			buffer_loc[0] = '\0';

			if (handle_virtual_object(jsve, ctx, buffer_loc, nbuffer))
			{
				added = add_to_buffer(buffer, nbuffer, buffer_loc, ir);
				if (!added)
				{
					delete buffer_loc;
					JS_FreeValue(ctx, jsve);
					return false;
				}
			}
			delete buffer_loc;
			JS_FreeValue(ctx, jsve);
		}
	}
	added = add_to_buffer(buffer, nbuffer, "]", ir);
	if (!added)
		return false;
	return true;
}


int aijsondb_query(const char* query, char* buffer, int nbuffer)
{
	std::lock_guard<std::mutex> lock(mtx);
	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewCustomContext(rt);
	int ifu = init_functions(ctx);
	if (ifu != 0) return -1;

	int init = aijsondb_index(ctx);
	if (init != 0) return -1;

	int ret = 0;
	{
		std::string query_str(query);
		JSValue jsv = JS_Eval(ctx, query_str.c_str(), query_str.size(), "<query>", JS_EVAL_TYPE_GLOBAL);
		//int32_t int_result;
		//JS_ToInt32(ctx, &int_result, jsv);
		//printf("ih==%d\n", int_result);
		if (JS_IsException(jsv)) {
			js_error_message(ctx, jsv, buffer, nbuffer);
			printf("%s\n", buffer);
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return -1;
		}
		else {
			JS_FreeValue(ctx, jsv);
		}
	}

	{
		const char* eres = "result";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			js_error_message(ctx, jsv, buffer, nbuffer);
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return -1;
		}
		else {

			bool is_virtual_object = is_virtual_result_object(ctx,jsv);
			bool is_virtual_array = is_virtual_result_array(ctx, jsv);
			if (is_virtual_object)
			{
				bool is_handled=handle_virtual_object(jsv, ctx, buffer, nbuffer);
				if (!is_handled)
					ret = -1;
				JS_FreeValue(ctx, jsv);
			}
			else if (is_virtual_array)
			{
				bool is_handled=handle_virtual_array(jsv, ctx, buffer, nbuffer);
				if (!is_handled)
					ret = -1;
				JS_FreeValue(ctx, jsv);
			}
			else
			{
				//JSClassID json_class_id=JS_GetClassID(jsv);
				//JSAtom jcn=JS_GetClassName(rt, json_class_id);
				//const char * gh=JS_AtomToCString(ctx, jcn);
				JSValue jjson = JS_JSONStringify(ctx, jsv, JS_UNDEFINED, JS_UNDEFINED);
				const char* jsh = JS_ToCString(ctx, jjson);
				//printf("GGGGGG\n");
				//printf("%s\n", jsh);
				buffer[0] = '\0';
				if (strlen(jsh) < nbuffer - 1) {
					strcpy(buffer, jsh);
				}
				else
				{
					ret = -1;
					//printf("Buffer too small for result\n");
				}
				JS_FreeValue(ctx, jjson);
				JS_FreeValue(ctx, jsv);
				//JS_FreeAtom(ctx, jcn);
				JS_FreeCString(ctx, jsh);
			}
			//JS_FreeCString(ctx, gh);
		}
	}
	//printf("Hello vor Ende\n");
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
	return ret;
}

int aijsondb_query_v0(const char* query, char* buffer, int nbuffer)
{
	std::lock_guard<std::mutex> lock(mtx);
	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewCustomContext(rt);
	int ifu=init_functions(ctx);
	if (ifu != 0) return -1;

	int init=aijsondb_index(ctx);
	if (init != 0) return -1;

	{
		std::string query_str(query);
		JSValue jsv = JS_Eval(ctx, query_str.c_str(),query_str.size(), "<query>", JS_EVAL_TYPE_GLOBAL);
		//int32_t int_result;
		//JS_ToInt32(ctx, &int_result, jsv);
		//printf("ih==%d\n", int_result);
		if (JS_IsException(jsv)) {
			js_error_message(ctx, jsv, buffer, nbuffer);
			printf("%s\n", buffer);
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return -1;
		}
		else {
			JS_FreeValue(ctx, jsv);
		}
	}

	{
		const char* eres = "JSON.stringify(result)";
		JSValue jsv = JS_Eval(ctx,eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			js_error_message(ctx, jsv, buffer, nbuffer);
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return -1;
		}
		else {
			const char* jsh = JS_ToCString(ctx, jsv);
			//printf("GGGGGG\n");
			//printf("%s\n", jsh);
			buffer[0] = '\0';
			if (strlen(jsh) < nbuffer - 1) {
				strcpy(buffer, jsh);
			}
			else
			{
				//printf("Buffer too small for result\n");
			}
			JS_FreeValue(ctx, jsv);
			JS_FreeCString(ctx, jsh);
		}
	}
	//printf("Hello vor Ende\n");
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
	return 0;
}


static JSValue js_print(JSContext* ctx, JSValueConst this_val,
	int argc, JSValueConst* argv) {
	for (int i = 0; i < argc; i++) {
		const char* str = JS_ToCString(ctx, argv[i]);
		if (!str) {
			return JS_EXCEPTION;
		}
		printf("%s", str);
		JS_FreeCString(ctx, str);
		if (i != argc - 1) {
			printf(" ");
		}
	}
	printf("\n");
	return JS_UNDEFINED;
}


static JSValue js_get_schema(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	std::string val = jobject_cache_schema;
	JSValue jres = JS_ParseJSON(ctx, val.c_str(), val.size(), "<bucket_object>");
	if (JS_IsException(jres)) {
		char buffer[1024];
		js_error_message(ctx, jres, buffer, 1024);
		printf("Error parsing JSON for bucket_object: %s\n", buffer);
	}
	return jres;
}

static JSValue js_get_bucket_object(JSContext* ctx, JSValueConst this_val,int argc, JSValueConst* argv) {
	if(argc<2)
		return JS_UNDEFINED;

	const char* bucket = JS_ToCString(ctx, argv[0]);
	if (!bucket) {
		return JS_EXCEPTION;
	}
	auto it = jobject_cache.find(bucket);
	if(it==jobject_cache.end())
	{
		JS_FreeCString(ctx, bucket);
		return JS_UNDEFINED;
	}
	int index = 0;
    int res=JS_ToInt32(ctx,&index,argv[1]);
	if (res<0)
	{
		JS_FreeCString(ctx, bucket);
		return JS_EXCEPTION;
	}

	if(index<0 || index>=jobject_cache[bucket].size())
	{
		JS_FreeCString(ctx, bucket);
		return JS_UNDEFINED;
	}
	
	std::string val = jobject_cache[bucket][index];
	JSValue jres =JS_ParseJSON(ctx, val.c_str(),val.size(), "<bucket_object>");
	if (JS_IsException(jres)) {
		char buffer[1024];
		js_error_message(ctx,jres, buffer,1024);
		printf("Error parsing JSON for bucket_object: %s\n", buffer);
	}
	return jres;
}

static JSValue js_get_buckets(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	
	JSValue arr = JS_NewArray(ctx);
	size_t i = 0;
	for (const auto& pair : jobject_cache) {
		JS_SetPropertyUint32(ctx, arr, i, JS_NewString(ctx,pair.first.c_str()));
		i++;
	}
	return arr;
}

static JSValue js_get_bucket_length(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	if (argc < 1)
		return JS_UNDEFINED;

	const char* bucket = JS_ToCString(ctx, argv[0]);
	if (!bucket) {
		return JS_EXCEPTION;
	}
	auto it = jobject_cache.find(bucket);
	if (it == jobject_cache.end())
	{
		JS_FreeCString(ctx, bucket);
		return JS_UNDEFINED;
	}

	int32_t ilen = (*it).second.size();
	JSValue len = JS_NewInt32(ctx,ilen);
	return len;
}

int load_cache(const char* filepath)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::ifstream is(filepath);
	if(!is) {
		last_error_message = "Error opening data file";
		return -1;
	}
	jobject_cache.clear();
	jsoncons::json_stream_cursor cursor(is);
	int level = 0;
	int level_array = 0;
    std::string current_key;
    jsoncons::json_decoder<jsoncons::json> decoder;

	for (; !cursor.done(); cursor.next())
	{
		const auto& event = cursor.current();
		switch (event.event_type())
		{
		case jsoncons::staj_event_type::begin_object:
		{
			
			if (level==1 && level_array == 1)
			{
				cursor.read_to(decoder);
			    jsoncons::json j = decoder.get_result();
				std::string sdata;
				j.dump(sdata);
				jobject_cache[current_key].push_back(sdata);
				//std::cout << "Object: " << sdata << std::endl;
			}
			else
			{
				level++;
			}
			break;
		}
		case jsoncons::staj_event_type::end_object:
		{
			level--;
			break;
		}
		case jsoncons::staj_event_type::begin_array:
		{

			level_array++;
			break;
		}
		case jsoncons::staj_event_type::end_array:
		{

			level_array--;
			break;
		}
		case jsoncons::staj_event_type::key:
		{
			if (level == 1)
			{
			    current_key=std::string(event.get<jsoncons::string_view>());
				jobject_cache[current_key] = {};
				//std::cout << level << std::endl;
				//std::cout << "Key: " << current_key << std::endl;
			}
			break;
		}
		}
	}
	return 0;
}

bool validate_object(const std::string& bucket,jsoncons::json& jobj, jsoncons::jsonschema::json_schema<jsoncons::json>& compiled)
{
	jsoncons::json jo(jobj);

	jsoncons::json_array<jsoncons::json> jarr;
	jarr.push_back(jo);
	jsoncons::json ja;
	ja[bucket.c_str()]=jarr;
	//std::string sh;
	//ja.dump(sh);
	//std::cout << "Validating object: " << sh << std::endl;
	bool valid = true;
	try
	{
		compiled.validate(ja);
	}
	catch (const std::exception& e)
	{
		//std::cout << e.what() << "\n";
		last_error_message = e.what();
		valid = false;
	}
	return valid;
}

void set_schema_version(jsoncons::json& schema)
{
	if (!schema.contains("$schema"))
	{
		schema["$schema"] = jsoncons::jsonschema::schema_version::draft202012();
	}
	else
	{
		std::string schema_version = schema["$schema"].as_string();
		if (schema_version != jsoncons::jsonschema::schema_version::draft4() &&
			schema_version != jsoncons::jsonschema::schema_version::draft6() &&
			schema_version != jsoncons::jsonschema::schema_version::draft7() &&
			schema_version != jsoncons::jsonschema::schema_version::draft201909() &&
			schema_version != jsoncons::jsonschema::schema_version::draft202012())
		{
			if (schema_version.find_first_of("http") == 0)
			{
				schema["$schema"] = jsoncons::jsonschema::schema_version::draft7();
			}
			else
			{
				schema["$schema"] = jsoncons::jsonschema::schema_version::draft202012();
			}
		}
	}
}

int load_cache_and_validate(const char* filepath)
{
	std::lock_guard<std::mutex> lock(mtx);
	std::ifstream is(filepath);
	if (!is) {
		last_error_message = "Error opening data file";
		return -1;
	}
	jobject_cache.clear();
	jsoncons::json_stream_cursor cursor(is);
	int level = 0;
	int level_array = 0;
	std::string current_key;
	jsoncons::json_decoder<jsoncons::json> decoder;

	if (jobject_cache_schema.size() == 0)
	{
		last_error_message = "Schema empty";
		return -1;
	}

	jsoncons::json schema = jsoncons::json::parse(jobject_cache_schema);
	set_schema_version(schema);
	

	jsoncons::jsonschema::json_schema<jsoncons::json> compiled = jsoncons::jsonschema::make_json_schema(std::move(schema));

	for (; !cursor.done(); cursor.next())
	{
		const auto& event = cursor.current();
		switch (event.event_type())
		{
		case jsoncons::staj_event_type::begin_object:
		{

			if (level == 1 && level_array == 1)
			{
				cursor.read_to(decoder);
				jsoncons::json j = decoder.get_result();
				if (!validate_object(current_key, j, compiled))
				{
					//last_error_message = "Object does not conform to schema";
					return -1;
				}
				std::string sdata;
				j.dump(sdata);
				jobject_cache[current_key].push_back(sdata);
				//std::cout << "Object: " << sdata << std::endl;
			}
			else
			{
				level++;
			}
			break;
		}
		case jsoncons::staj_event_type::end_object:
		{
			level--;
			break;
		}
		case jsoncons::staj_event_type::begin_array:
		{

			level_array++;
			break;
		}
		case jsoncons::staj_event_type::end_array:
		{

			level_array--;
			break;
		}
		case jsoncons::staj_event_type::key:
		{
			if (level == 1)
			{
				current_key = std::string(event.get<jsoncons::string_view>());
				jobject_cache[current_key] = {};
				//std::cout << level << std::endl;
				//std::cout << "Key: " << current_key << std::endl;
			}
			break;
		}
		}
	}
	return 0;
}

int init_functions(JSContext* ctx)
{
	JSValue global_obj = JS_GetGlobalObject(ctx);

	JSValue get_bucket_object = JS_NewCFunction(ctx, js_get_bucket_object, "domino_bucket_object", 2);
	JS_SetPropertyStr(ctx, global_obj, "domino_bucket_object", get_bucket_object);
	
	JSValue get_buckets = JS_NewCFunction(ctx, js_get_buckets, "domino_buckets", 0);
	JS_SetPropertyStr(ctx, global_obj, "domino_buckets", get_buckets);
	
	JSValue get_len_bucket = JS_NewCFunction(ctx, js_get_bucket_length, "domino_bucket_length", 1);
	JS_SetPropertyStr(ctx, global_obj, "domino_bucket_length", get_len_bucket);

	JSValue get_schema = JS_NewCFunction(ctx, js_get_schema , "domino_schema", 0);
	JS_SetPropertyStr(ctx, global_obj, "domino_schema", get_schema);
	
	JS_FreeValue(ctx, global_obj);
	return 0;
}


int aijsondb_query_test()
{
	aijsondb_load_data("C:/NHKI/aijsondb/data/500 KB_V2.json", "C:/NHKI/aijsondb/data/employeeSchemaDescription_V2.json");
	{
	  std::cout << "buckets: " << jobject_cache.size() << std::endl;
	  std::cout << "employees" << jobject_cache["employees"].size() << std::endl;
	  std::cout << "employe[100]" << jobject_cache["employees"][100] << std::endl;
	}
	{
		std::lock_guard<std::mutex> lock(mtx);

//		jobject_cache["employees"] = { "{\"id\":\"E0001\",\"name\":\"Berta\"}" };

		JSRuntime* rt;
		JSContext* ctx;
		rt = JS_NewRuntime();
		ctx = JS_NewCustomContext(rt);
		const int nbuffer = 1024 * 10;
		char buffer[nbuffer];
		//printf("%s\n", domino_data);

		init_functions(ctx);

		{
			std::string query = "domino_buckets()";
			JSValue jsv = JS_Eval(ctx, query.c_str(), query.size(), "<test>", JS_EVAL_TYPE_GLOBAL);
			//int32_t int_result;
			//JS_ToInt32(ctx, &int_result, jsv);
			//printf("ih==%d\n", int_result);
			if (JS_IsException(jsv)) {
				js_error_message(ctx, jsv, buffer, nbuffer);
				JS_FreeValue(ctx, jsv);
				JS_FreeContext(ctx);
				JS_FreeRuntime(rt);
				return -1;
			}
			else {
				JSValue jsonh = JS_JSONStringify(ctx, jsv, JS_UNDEFINED, JS_UNDEFINED);
				const char* jsh = JS_ToCString(ctx,jsonh);
				printf("%s\n", jsh);
				JS_FreeCString(ctx, jsh);
				JS_FreeValue(ctx, jsonh);
				JS_FreeValue(ctx, jsv);
			}
		}

		{
			std::string query = "domino_bucket_length('employees')";
			JSValue jsv = JS_Eval(ctx, query.c_str(), query.size(), "<test>", JS_EVAL_TYPE_GLOBAL);
			//int32_t int_result;
			//JS_ToInt32(ctx, &int_result, jsv);
			//printf("ih==%d\n", int_result);
			if (JS_IsException(jsv)) {
				js_error_message(ctx, jsv, buffer, nbuffer);
				JS_FreeValue(ctx, jsv);
				JS_FreeContext(ctx);
				JS_FreeRuntime(rt);
				return -1;
			}
			else {
				int32_t int_result=0;
				int ih=JS_ToInt32(ctx,&int_result, jsv);
				printf("%d\n", int_result);
				JS_FreeValue(ctx, jsv);
			}
		}
	

		{
			std::string query = "domino_bucket_object('employees',0)";
			JSValue jsv = JS_Eval(ctx, query.c_str(), query.size(), "<test>", JS_EVAL_TYPE_GLOBAL);
			//int32_t int_result;
			//JS_ToInt32(ctx, &int_result, jsv);
			//printf("ih==%d\n", int_result);
			if (JS_IsException(jsv)) {
				js_error_message(ctx, jsv, buffer, nbuffer);
				JS_FreeValue(ctx, jsv);
				JS_FreeContext(ctx);
				JS_FreeRuntime(rt);
				return -1;
			}
			else {
				JSValue jsonh = JS_JSONStringify(ctx, jsv, JS_UNDEFINED, JS_UNDEFINED);
				const char* jsh = JS_ToCString(ctx, jsonh);
				printf("%s\n", jsh);
				JS_FreeCString(ctx, jsh);
				JS_FreeValue(ctx, jsonh);
				JS_FreeValue(ctx, jsv);
			}
		}

		{
			std::string query = "domino_schema()";
			JSValue jsv = JS_Eval(ctx, query.c_str(), query.size(), "<test>", JS_EVAL_TYPE_GLOBAL);
			//int32_t int_result;
			//JS_ToInt32(ctx, &int_result, jsv);
			//printf("ih==%d\n", int_result);
			if (JS_IsException(jsv)) {
				js_error_message(ctx, jsv, buffer, nbuffer);
				JS_FreeValue(ctx, jsv);
				JS_FreeContext(ctx);
				JS_FreeRuntime(rt);
				return -1;
			}
			else {
				JSValue jsonh = JS_JSONStringify(ctx, jsv, JS_UNDEFINED, JS_UNDEFINED);
				const char* jsh = JS_ToCString(ctx, jsonh);
				printf("%s\n", jsh);
				JS_FreeCString(ctx, jsh);
				JS_FreeValue(ctx, jsonh);
				JS_FreeValue(ctx, jsv);
			}
		}

		//printf("Hello vor Ende\n");
		JS_FreeContext(ctx);
		JS_FreeRuntime(rt);
	}

	{
		const int nbuffer = 1024;
		char buffer[nbuffer];
		aijsondb_query("let result=data.employees.length;", buffer, nbuffer);
	}
	return 0;
}