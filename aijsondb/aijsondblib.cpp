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
#include "aijsondbimporter.hpp"
#include "aijsondblib.h"
#include "quickjs.h"
#include "quickjs-libc.h"
#include "aijsondbindex.h"
#include "aijsondbresolver.h"
#include "aijsondbloadwithimporter.h"
#include <filesystem>


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

std::map<std::string, std::vector<std::string>>* get_object_cache() {
	return &jobject_cache;
}

std::string* get_jobject_cache_schema()
{
	return &jobject_cache_schema;
}


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

int aijsondb_save_data(const char* filepath_data)
{
		//std::lock_guard<std::mutex> lock(mtx);
		if (std::filesystem::exists(filepath_data)) {
			last_error_message = "File already exists: ";
			last_error_message.append(filepath_data);
			return -1;
		}


		std::ofstream file(filepath_data, std::ios::out | std::ios::binary);
		if (!file) {
			last_error_message = "Error opening schema file";
			return -1;
		}
		size_t ibucket = 0;
		size_t nbucket=jobject_cache.size();
		file << "{";
		for (auto kv : jobject_cache){
			file << "\"" << kv.first << "\" : [" << "\n";
			size_t i=0;
			for (auto entity : kv.second)
			{
				file << entity;
				if (i + 1 < kv.second.size())
				{
					file << ",";
				}
				file << "\n";
				i++;
			}
			if (ibucket + 1 < nbucket)
			{
				file << "]," << "\n";
			}
			else
			{
				file << "]" << "\n";
			}
			ibucket++;
	    }
		file << "}";
		file.close();
		return 0;
	//printf("Data loaded successfully:\n%s\n", domino_data);
}

int aijsondb_load_data_with_cache(
	const char* filename,
	const char* json_filename,
	const char* schema
) {

	if (std::filesystem::exists(json_filename))
		return aijsondb_load_data(json_filename, schema);

	if (!std::filesystem::exists(filename)) {
		last_error_message = "File does not exists: ";
		last_error_message.append(filename);
		return -1;
	}

	if (std::filesystem::exists(json_filename)) {
		last_error_message = "JSON data file alread exists: ";
		last_error_message.append(json_filename);
		return -1;
	}

	if (std::filesystem::exists(schema)) {
		last_error_message = "JSON schema file alread exists: ";
		last_error_message.append(schema);
		return -1;
	}

	std::filesystem::path fpx(filename);
	std::string ext = fpx.extension().string();
	IBulkImporter* importer = get_importer(ext);
	if (importer == nullptr)
	{
		last_error_message = "No importer for: ";
		last_error_message.append(filename);
		return -1;
	}



    std::string error;
	if (!importer->import(filename, jobject_cache, jobject_cache_schema, error))
	{
		last_error_message = error;
		return -1;
	}

	if (aijsondb_save_data(json_filename) != 0)
		return -1;


	std::ofstream file(schema, std::ios::out | std::ios::binary);
	if (!file) {
		last_error_message = "Error opening schema file";
		return -1;
	}
	
	file << jobject_cache_schema;

	file.close();

	return 0;
}




int init_functions(JSContext* ctx);

const char* get_bucket_object(const char* bucket, int index)
{
    std::string bucket_str(bucket);

	if(jobject_cache.find(bucket_str) == jobject_cache.end())
	{
		return "";
	}

	if (jobject_cache[bucket].size() <= index)
	{
		return "";
	}

	return jobject_cache[bucket][index].c_str();
}

const char* get_bucket_name_from_index(int index) {
	int i = 0;
	for (const auto& pair : jobject_cache) {
		if (i == index)
		{
			return pair.first.c_str();
		}
		i++;
	}
	return "";
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
			//	ResultRows rows;
			//	jsoncons::json j;
			//	walk_objects_and_resolve(ctx,jsv,nullptr,rows,j);
				jsoncons::json j;
				toJsonWithVirtual(ctx, jsv, j);
				if (!j.is_null())
				{
					std::stringstream sst;
					sst << j;
					std::string res = sst.str();
					//std::cout << "Serialized JSON: " << sst.str() << std::endl;
					JS_FreeValue(ctx, jsv);
					buffer[0] = '\0';
					if (res.size() < nbuffer - 1) {
						strcpy(buffer, res.c_str());
					}
					else
					{
						ret = -1;
						const char* error_message = "Buffer too small for result";
						if (strlen(error_message) < nbuffer - 1) {
							strcpy(buffer,error_message);
						}
						//printf("Buffer too small for result\n");
					}
				}
				else
				{
					ret = -1;
					const char* error_message = "result is undefined";
					if (strlen(error_message) < nbuffer - 1) {
						strcpy(buffer, error_message);
					}
				}
			//JS_FreeCString(ctx, gh);
		}
	}
	//printf("Hello vor Ende\n");
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
	return ret;
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

	JSValue get_bucket_object = JS_NewCFunction(ctx, js_get_bucket_object, "aijsondb_bucket_object", 2);
	JS_SetPropertyStr(ctx, global_obj, "aijsondb_bucket_object", get_bucket_object);
	
	JSValue get_buckets = JS_NewCFunction(ctx, js_get_buckets, "aijsondb_buckets", 0);
	JS_SetPropertyStr(ctx, global_obj, "aijsondb_buckets", get_buckets);
	
	JSValue get_len_bucket = JS_NewCFunction(ctx, js_get_bucket_length, "aijsondb_bucket_length", 1);
	JS_SetPropertyStr(ctx, global_obj, "aijsondb_bucket_length", get_len_bucket);

	JSValue get_schema = JS_NewCFunction(ctx, js_get_schema , "aijsondb_schema", 0);
	JS_SetPropertyStr(ctx, global_obj, "aijsondb_schema", get_schema);
	
	JS_FreeValue(ctx, global_obj);
	return 0;
}

std::mutex mtx_importer;
static std::map<std::string,std::unique_ptr<IBulkImporter>> importers;

bool register_importer(std::unique_ptr<IBulkImporter>& importer) {
	
	std::lock_guard<std::mutex> lock(mtx_importer);

	if (importer == nullptr)
		return false;

	std::string ending = importer->ending();
	if (ending.size() == 0)
		return false;
	importers[ending] = std::move(importer);
	return true;
}

IBulkImporter* get_importer(const std::string& ending)
{
	std::lock_guard<std::mutex> lock(mtx_importer);
	if (ending.size() == 0)
		return nullptr;

	auto ef = importers.find(ending);
	if (ef == importers.end())
		return nullptr;
	IBulkImporter* hl= ef->second.get();
	return hl;
}

void clear_importer(std::unique_ptr<IBulkImporter>& importer) {

	std::lock_guard<std::mutex> lock(mtx_importer);
	importers.clear();
}
