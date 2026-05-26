#ifndef AIJSONDBRESOLVER_H
#define AIJSONDBRESOLVER_H
enum class ResultType
{
	Single,
	Array,
	Dictinary
};
/*
class ResultRows {
	public:
		ResultType	resultType;
		std::vector<jsoncons::json> rows;
		std::map<std::string, int> dictinary_rows;
};
bool is_virtual_result_object(JSContext* ctx, const JSValue& jsv);
void walk_objects_and_resolve(JSContext* ctx, const JSValue& jsv, const JSValue* parent, ResultRows& rows, jsoncons::json& j);
*/
//jsoncons::json& toJson(JSContext* ctx, const JSValue& jsv);
void toJson2(JSContext* ctx, const JSValue& jsv, jsoncons::json& j);
void toJsonWithVirtual(JSContext* ctx, const JSValue& jsv, jsoncons::json& j);
bool is_virtual_result_object(JSContext* ctx, const JSValue& jsv);
size_t save_result(std::map<size_t, jsoncons::json>& mapj, jsoncons::json& j);
bool get_result(std::map<size_t, jsoncons::json>& mapj, size_t result_set_index, size_t index, std::string& bucket, jsoncons::json& value, bool& isArray);

#endif