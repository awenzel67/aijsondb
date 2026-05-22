#include "quickjs.h"
#include "quickjs-libc.h"
#include <string>
#include <vector>
#include <jsoncons/json.hpp>
#include "aijsondblib.h"
#include "aijsondbindex.h"
#include "aijsondbresolver.h"

bool isInteger(double value) {
	// Use an epsilon tolerance to handle floating-point rounding errors
	constexpr double epsilon = std::numeric_limits<double>::epsilon();
	return std::fabs(value - std::round(value)) < epsilon;		
}

void toJson2(JSContext* ctx, const JSValue& jsv,jsoncons::json& j)
{
	if (JS_IsString(jsv))
	{
		const char* str_val = JS_ToCString(ctx, jsv);
		j = jsoncons::json(str_val);
		JS_FreeCString(ctx, str_val);
	}
	else if (JS_IsNumber(jsv))
	{
		int tag = JS_VALUE_GET_TAG(jsv);
		//bool ko= tag == JS_TAG_INT || JS_TAG_IS_FLOAT64(tag);
		if (JS_TAG_IS_FLOAT64(tag)) {
			double num_val;
			if (JS_ToFloat64(ctx, &num_val, jsv) >= 0)
			{
				if (isInteger(num_val)) {
					long ni=std::lround(num_val);
					j = jsoncons::json(ni);
				} else {
					j = jsoncons::json(num_val);
				}
			}
		}
		else if (tag==JS_TAG_INT) {
			int32_t int_val;
			if (JS_ToInt32(ctx, &int_val, jsv) >= 0)
			{
				j = jsoncons::json(int_val);
			}
		}
	}
	else if (JS_IsArray(jsv))
	{
		j=jsoncons::json(jsoncons::json_array_arg, { });
		int64_t length;
		int gu = JS_GetLength(ctx, jsv, &length);
		for (int i = 0; i < length; i++)
		{
			JSValue jsve = JS_GetPropertyUint32(ctx, jsv, i);
			if (!JS_IsException(jsve))
			{
				jsoncons::json jnew;
				toJson2(ctx, jsve, jnew);
				j.push_back(jnew);
				JS_FreeValue(ctx, jsve);
			}
		}
	}
	else if (JS_IsObject(jsv))
	{
		JSPropertyEnum* properties;
		uint32_t nproperties;
		j = jsoncons::json(jsoncons::json_object_arg);
		int gpn = JS_GetOwnPropertyNames(ctx, &properties, &nproperties, jsv, JS_GPN_STRING_MASK);
		for (uint32_t i = 0; i < nproperties; i++)
		{
			const char* prop_name = JS_AtomToCString(ctx, properties[i].atom);
			JSValue jsve = JS_GetProperty(ctx, jsv, properties[i].atom);
			if (!JS_IsException(jsve)){
				jsoncons::json jnew;
				toJson2(ctx, jsve, jnew);
				j[prop_name] = jnew;
				JS_FreeValue(ctx, jsve);
			}	
			JS_FreeAtom(ctx, properties[i].atom);
		}
		js_free(ctx, properties);
	}
}

void toJsonWithVirtual(JSContext* ctx, const JSValue& jsv, jsoncons::json& j)
{
	if (JS_IsString(jsv))
	{
		const char* str_val = JS_ToCString(ctx, jsv);
		j = jsoncons::json(str_val);
		JS_FreeCString(ctx, str_val);
	}
	else if (JS_IsBool(jsv))
	{
		bool bool_val = JS_ToBool(ctx, jsv);
		j = jsoncons::json(bool_val);
	}
	else if (JS_IsUndefined(jsv))
	{
		j = jsoncons::json(nullptr);
	}
	else if (JS_IsDate(jsv))
	{
		JSValue jjson = JS_JSONStringify(ctx, jsv, JS_UNDEFINED, JS_UNDEFINED);
		const char* jsh = JS_ToCString(ctx, jjson);
		if (jsh != nullptr)
		{
			size_t lstr = strlen(jsh);
			if (lstr > 4)
			{
				std::string requote(jsh+1,lstr-2);
				j = jsoncons::json( requote);
			}
			else
			{
				j = jsoncons::json(nullptr);
			}
		}
		else
		{
			j = jsoncons::json(nullptr);
		}
		JS_FreeCString(ctx, jsh);
	}
	else if (JS_IsNumber(jsv))
	{
		int tag = JS_VALUE_GET_TAG(jsv);
		//bool ko= tag == JS_TAG_INT || JS_TAG_IS_FLOAT64(tag);
		if (JS_TAG_IS_FLOAT64(tag)) {
			double num_val;
			if (JS_ToFloat64(ctx, &num_val, jsv) >= 0)
			{
				if (isInteger(num_val)) {
					long ni = std::lround(num_val);
					j = jsoncons::json(ni);
				}
				else {
					j = jsoncons::json(num_val);
				}
			}
		}
		else if (tag == JS_TAG_INT) {
			int32_t int_val;
			if (JS_ToInt32(ctx, &int_val, jsv) >= 0)
			{
				j = jsoncons::json(int_val);
			}
		}
	}
	else if (JS_IsArray(jsv))
	{
		j = jsoncons::json(jsoncons::json_array_arg, { });
		int64_t length;
		int gu = JS_GetLength(ctx, jsv, &length);
		for (int i = 0; i < length; i++)
		{
			JSValue jsve = JS_GetPropertyUint32(ctx, jsv, i);
			if (!JS_IsException(jsve))
			{
				jsoncons::json jnew;
				toJsonWithVirtual(ctx, jsve, jnew);
				j.push_back(jnew);
				JS_FreeValue(ctx, jsve);
			}
		}
	}
	else if (JS_IsObject(jsv))
	{
		if (is_virtual_result_object(ctx, jsv))
		{
			int eindex_ok = -1;
			int bindex_ok = -1;
			JSValue jsvo = JS_GetPropertyStr(ctx, jsv, "eindex");
			if (!JS_IsException(jsvo))
			{
				int32_t eindex;
				if (JS_ToInt32(ctx, &eindex, jsvo) >= 0)
				{
					eindex_ok = eindex;
				}
				JS_FreeValue(ctx, jsvo);
			}
			JSValue jsvb = JS_GetPropertyStr(ctx, jsv, "bindex");
			if (!JS_IsException(jsvb))
			{
				int32_t bindex;
				if (JS_ToInt32(ctx, &bindex, jsvb) >= 0)
				{
					bindex_ok = bindex;
				}
				JS_FreeValue(ctx, jsvb);
			}
			if (bindex_ok >= 0 && eindex_ok >= 0)
			{
				std::string bucket_name = get_bucket_name_from_index(bindex_ok);
				std::string val = get_bucket_object(bucket_name.c_str(), eindex_ok);
				j = jsoncons::json::parse(val);
			}
			else
			{
				j = jsoncons::json(nullptr);
			}
		}
		else
		{
			JSPropertyEnum* properties;
			uint32_t nproperties;
			j = jsoncons::json(jsoncons::json_object_arg);
			int gpn = JS_GetOwnPropertyNames(ctx, &properties, &nproperties, jsv, JS_GPN_STRING_MASK);
			for (uint32_t i = 0; i < nproperties; i++)
			{
				const char* prop_name = JS_AtomToCString(ctx, properties[i].atom);
				JSValue jsve = JS_GetProperty(ctx, jsv, properties[i].atom);
				if (!JS_IsException(jsve)) {
					jsoncons::json jnew;
					toJsonWithVirtual(ctx, jsve, jnew);
					j[prop_name] = jnew;
					JS_FreeValue(ctx, jsve);
				}
				JS_FreeAtom(ctx, properties[i].atom);
			}
			js_free(ctx, properties);
		}
	}
}


size_t save_result(std::map<size_t,jsoncons::json>& mapj, jsoncons::json& j)
{
	if (mapj.size() == 0)
	{
		mapj[0] = j;
		return 0;
	}

	size_t max_index = 0;
	for (const auto& kv : mapj)
	{
		if(kv.first> max_index)
		{
			max_index = kv.first;
		}
	}
	max_index++;
	mapj[max_index] = j;
	return max_index;
}

bool get_result(std::map<size_t, jsoncons::json>& mapj,size_t result_set_index, size_t index,std::string& bucket, jsoncons::json& value,bool& isArray)
{
	bucket = "";
	isArray = false;
	value = jsoncons::json::null();

	auto result = mapj.find(result_set_index);
	if (result == mapj.end())
		return false;
    

	auto& result_json = result->second;
	if(result_json.is_array())
	{
		isArray = true;
		if (index < result_json.size())
		{
			value = result_json[index];
			return true;
		}
	}
	else if (result_json.is_object())
	{
		bool rootHasBuckets = false;
		{
			size_t index_from = 0;
			size_t index_to = 0;
			for (auto& kv : result_json.object_range())
			{
				if (kv.value().is_array())
				{
					rootHasBuckets = true;
					break;
				}
			}
		}
		
		if (rootHasBuckets)
		{
			size_t index_from = 0;
			size_t index_to = 0;
			for (auto& kv : result_json.object_range())
			{
				if (kv.value().is_array())
				{
					index_to = index_from + kv.value().size();
					if (index_from <= index && index < index_to)
					{
						bucket = kv.key();
						value = kv.value()[index - index_from];
						isArray = true;
						return true;
					}
				}
				else
				{
					index_to = index_from + 1;
					if (index_from <= index && index < index_to)
					{
						bucket = kv.key();
						value = kv.value();
						return true;
					}
				}
				index_from = index_to;
			}
		}
		else
		{
			if (index > 0) {
				return false;
			}
			else
			{
				value = result_json;
				return true;
			}
		}
	}
	else
	{
		if (index > 0) {
			return false;
		}
		else
		{
			value = result_json;
			return true;
		}
	}
	return false;
}


void test_toJson(JSContext* ctx, const JSValue& jsv)
{
	jsoncons::json j;
	toJson2(ctx, jsv, j);
}

#if false
jsoncons::json& toJson(JSContext* ctx, const JSValue& jsv)
{
	if (JS_IsString(jsv))
	{
		const char* str_val = JS_ToCString(ctx, jsv);
		jsoncons::json j(str_val);
		JS_FreeCString(ctx, str_val);
		return j;
	}
	else if (JS_IsNumber(jsv))
	{
		double num_val;
		if (JS_ToFloat64(ctx, &num_val, jsv) >= 0)
		{
			jsoncons::json j(num_val);
			return j;
		}
	}
	else if (JS_IsBool(jsv))
	{
		bool bool_val = JS_ToBool(ctx, jsv);
		jsoncons::json j(bool_val);
		return j;
	}
	else if (JS_IsNull(jsv))
	{
		jsoncons::json j(nullptr);
		return j;
	}
	else if (JS_IsArray(jsv))
	{
		jsoncons::json j(nullptr);
		return j;
	}
	else if (JS_IsObject(jsv))
	{
		jsoncons::json j(nullptr);
		return j;
	}
	else
	{
		// for other types, we return null
		jsoncons::json j(nullptr);
		return j;
	}
}
#endif

#if false
void walk_objects_and_resolve(JSContext* ctx, const JSValue& jsv,const JSValue* parent, ResultRows& rows, jsoncons::json& j)
{
	bool is_first_level = (parent == nullptr);
	bool is_array = JS_IsArray(jsv);
	if (is_array)
	{
		jsoncons::json janew(jsoncons::json_array_arg, { });
		if (!is_first_level)
		{
			j = janew;
		}
		int64_t length;
		int gu = JS_GetLength(ctx, jsv, &length);
		for (int i=0;i<length;i++)
		{
			JSValue jsve = JS_GetPropertyUint32(ctx, jsv, i);
			if (!JS_IsException(jsve))
			{
				if (is_first_level)
				{
					// if the first level is an array, we consider it as a result set
					// and we add each element to the rows
					jsoncons::json jnew;
					walk_objects_and_resolve(ctx, jsve, &jsv, rows, jnew);
					rows.rows.push_back(jnew);
				}
				else
				{
					jsoncons::json jnew;
					walk_objects_and_resolve(ctx, jsve, &jsv, rows, jnew);
					j.push_back(jnew);
				}
				JS_FreeValue(ctx, jsve);
			}
		}
	}

	bool is_object = JS_IsObject(jsv);
	if (is_object)
	{
		if (is_virtual_result_object(ctx,jsv))
		{
			int eindex_ok = -1;
			int bindex_ok = -1;
			JSValue jsvo = JS_GetPropertyStr(ctx, jsv, "eindex");
			if (!JS_IsException(jsvo))
			{
				int32_t eindex;
				if (JS_ToInt32(ctx, &eindex, jsvo) >= 0)
				{
					eindex_ok = eindex;
				}
				JS_FreeValue(ctx, jsvo);
			}
			JSValue jsvb = JS_GetPropertyStr(ctx, jsv, "bindex");
			if (!JS_IsException(jsvb))
			{
				int32_t bindex;
				if (JS_ToInt32(ctx, &bindex, jsvb) >= 0)
				{
					bindex_ok = bindex;
				}
				JS_FreeValue(ctx, jsvb);
			}
			if(bindex_ok >= 0 && eindex_ok >= 0)
			{
				std::string bucket_name = get_bucket_name_from_index(bindex_ok);
				std::string val = get_bucket_object(bucket_name.c_str(), eindex_ok);
			}
		}
		else {
			jsoncons::json janew();
			if (!is_first_level)
			{
				j = janew;
			}
			JSPropertyEnum* properties;
			uint32_t nproperties;
			int gpn = JS_GetOwnPropertyNames(ctx, &properties, &nproperties, jsv, JS_GPN_STRING_MASK);
			for (uint32_t i = 0; i < nproperties; i++)
			{
				const char* prop_name = JS_AtomToCString(ctx, properties[i].atom);
				JSValue jsve = JS_GetProperty(ctx, jsv, properties[i].atom);
				if (!JS_IsException(jsve))
				{
					jsoncons::json jnew;
					walk_objects_and_resolve(ctx, jsve, &jsv, rows, jnew);

					if (is_first_level)
					{
						rows.dictinary_rows[std::string(prop_name)] = rows.rows.size();
						rows.rows.push_back(jnew);
					}
					else
					{
					}

					JS_FreeValue(ctx, jsve);
				}
				JS_FreeAtom(ctx, properties[i].atom);
			}
			js_free(ctx, properties);
		}
	}
	else
	{ 
	    if (JS_IsString(jsv))
		{
			const char* str_val = JS_ToCString(ctx, jsv);
			j = jsoncons::json(str_val);
			JS_FreeCString(ctx, str_val);
		}
		else if (JS_IsNumber(jsv))
		{
			double num_val;
			if (JS_ToFloat64(ctx, &num_val, jsv) >= 0)
			{
				j = jsoncons::json(num_val);
			}
		}
		else if (JS_IsBool(jsv))
		{
			bool bool_val = JS_ToBool(ctx, jsv);
			j = jsoncons::json(bool_val);
		}
		else if (JS_IsNull(jsv))
		{
			j = jsoncons::json(nullptr);
		}
	}
}
#endif
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


