#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "../aijsondb/aijsondblib.h"
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include "quickjs.h"
#include "../aijsondb/aijsondbresolver.h"
#include "../aijsondbexcel/aijsondbimportexcel.h"
#include <sstream>
#include <filesystem>
#include <memory>


const char* test_data_dir()
{
	return TEST_DATA_DIR;
}

TEST_CASE("Test Domino Load", "[domino]") {
	std::string path_data=test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(),path_schema.c_str());
    REQUIRE(res==0);
	REQUIRE(strlen(aijsondb_last_error()) == 0);
}


const char* query_test1 = "var result = null;\n\n// Iterate through each employee\nfor (var i = 0; i < data.employees.length; i++) {\n    var employee = data.employees[i];\n    var projects = employee.profile.projects || [];\n    \n    // Iterate through each project of the employee\n    for (var j = 0; j < projects.length; j++) {\n        var project = projects[j];\n        \n        // Check if the project name matches\n        if (project.name === \"Incubate World-Class Schemas\") {\n            result = employee;\n            break;\n        }\n    }\n    \n    if (result !== null) {\n        break;\n    }\n};result";
const char* query_test2 = "var result = data.employees[101];result";

int aijsondb_query(const char* query, char* buffer, int nbuffer);

TEST_CASE("Test Domino query bucket object", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	char buffer[1024];
	res = aijsondb_query(query_test1,buffer,1024);
	std::cout << "Query result: " << buffer << std::endl;
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		REQUIRE(j["id"]=="E00001");
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
	REQUIRE(res == 0);
}


const char* query_test3 = "var result = data.employees;result";
TEST_CASE("Test Domino query bucket array", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 1000;
	char* buffer= new char[nbuffer];
	res = aijsondb_query(query_test3, buffer,nbuffer);
	//std::cout << "Query result: " << buffer << std::endl;
	REQUIRE(res == 0);
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		REQUIRE(j.size()==201);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
	delete buffer;
}

const char* query_test4 = "var result=data.KitalisteVer_ffentlichung.length";
TEST_CASE("Test Domino query excel", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	std::string pd = "C:/del/output_utf8.json";
	std::string ps = "C:/del/output_utf8_schema.json";
	int res = aijsondb_load_data(pd.c_str(), ps.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 1000;
	char* buffer = new char[nbuffer];
	res = aijsondb_query(query_test4, buffer, nbuffer);
	std::cout << "Query result: " << buffer << std::endl;
	REQUIRE(res == 0);
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		int count = j.as_integer<int>();
		REQUIRE(count == 2917);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
	delete buffer;
}


TEST_CASE("Test Domino query excel load", "[domino]") {
	IBulkImporter* o= new ExcelImporter();
	std::unique_ptr<IBulkImporter> op(o);
	register_importer(op);
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	std::string pd = "C:/del/output2_utf8.json";
	if (std::filesystem::exists(pd))
	{
		std::filesystem::remove(pd);
	}
	std::string ps = "C:/del/output2_utf8_schema.json";
	if (std::filesystem::exists(ps))
	{
		std::filesystem::remove(ps);
	}

	std::string filename = "kitaliste-nov-2025.xlsx";
	std::string pathk="C:/NHKI/data/talktodataexcel/" + filename;
	int res = aijsondb_load_data_with_cache(pathk.c_str(),pd.c_str(), ps.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 1000;
	char* buffer = new char[nbuffer];
	res = aijsondb_query(query_test4, buffer, nbuffer);
	std::cout << "Query result: " << buffer << std::endl;
	REQUIRE(res == 0);
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		int count = j.as_integer<int>();
		REQUIRE(count == 2917);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
	delete buffer;
}



TEST_CASE("Test Domino No Load Data", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V2X.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == -1);
	REQUIRE(strlen(aijsondb_last_error()) > 0);
}

TEST_CASE("Test Domino No Load Schema", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2X.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == -1);
	REQUIRE(strlen(aijsondb_last_error()) > 0);
}

bool validate_object(const std::string& bucket, jsoncons::json& jobj, jsoncons::jsonschema::json_schema<jsoncons::json>& compiled);

void set_schema_version(jsoncons::json& schema);

TEST_CASE("Test Domino Correct Schema", "[domino]") {

	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	std::ifstream bfile(path_schema, std::ios::in | std::ios::binary);
	std::ostringstream st;
	st << bfile.rdbuf(); // Ef

	std::string schema_str = st.str();
	jsoncons::json schema = jsoncons::json::parse(schema_str);
	set_schema_version(schema);
	REQUIRE(schema["$schema"]==jsoncons::jsonschema::schema_version::draft7());
}


TEST_CASE("Test Domino Validate", "[domino]") {
	
	std::string path_data = test_data_dir();
	path_data += "test.json";
	std::ifstream afile(path_data, std::ios::in | std::ios::binary);
	std::ostringstream ss;
	ss << afile.rdbuf(); // Ef
	jsoncons::json jo = jsoncons::json::parse(ss.str());


	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	std::ifstream bfile(path_schema, std::ios::in | std::ios::binary);
	std::ostringstream st;
	st << bfile.rdbuf(); // Ef

	std::string schema_str = st.str();
	jsoncons::json schema = jsoncons::json::parse(schema_str);
	set_schema_version(schema);	


	jsoncons::jsonschema::json_schema<jsoncons::json> compiled = jsoncons::jsonschema::make_json_schema(std::move(schema));

	const char* bucket = "employees";
	jsoncons::json jso=jo[bucket][0];
	bool res = validate_object(bucket, jso, compiled);
	REQUIRE(res);
}


TEST_CASE("Test Domino buckets index", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	char buffer[1024];
	std::string query = R"( 
let result=[data.employees[10].bindex,data.employees[10].eindex];
)";
	res = aijsondb_query(query.c_str(), buffer, 1024);
	std::cout << "Query result: " << buffer << std::endl;
	std::string sres(buffer);
	REQUIRE(res == 0);
	REQUIRE(sres == "[0,10]");
}



TEST_CASE("Test aijsondb query issue 4", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 100;
	char buffer[nbuffer];
	std::string query = R"( 
var result = [];
for (var i = 0; i < data.employees.length; i++) {
    if (data.employees[i].name.includes("Smith")) {
        result.push(data.employees[i]);
    }
}
)";
	res = aijsondb_query(query.c_str(), buffer, nbuffer);
	REQUIRE(res == 0);
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		REQUIRE(j.size() == 4);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
}

/*
{'firstTry': False,
 'isOk': True,
 'answer': '{"employees": [{"eindex": 5, "bindex": 0}, {"eindex": 8, "bindex": 0}, {"eindex": 9, "bindex": 0}, {"eindex": 13, "bindex": 0}, {"eindex": 18, "bindex": 0}, {"eindex": 22, "bindex": 0}, {"eindex": 34, "bindex": 0}, {"eindex": 36, "bindex": 0}, {"eindex": 37, "bindex": 0}, {"eindex": 46, "bindex": 0}, {"eindex": 48, "bindex": 0}, {"eindex": 49, "bindex": 0}, {"eindex": 52, "bindex": 0}, {"eindex": 55, "bindex": 0}, {"eindex": 67, "bindex": 0}, {"eindex": 71, "bindex": 0}, {"eindex": 86, "bindex": 0}, {"eindex": 117, "bindex": 0}, {"eindex": 122, "bindex": 0}, {"eindex": 127, "bindex": 0}, {"eindex": 134, "bindex": 0}, {"eindex": 146, "bindex": 0}, {"eindex": 158, "bindex": 0}, {"eindex": 163, "bindex": 0}, {"eindex": 167, "bindex": 0}, {"eindex": 169, "bindex": 0}, {"eindex": 177, "bindex": 0}, {"eindex": 190, "bindex": 0}, {"eindex": 199, "bindex": 0}], "maxExperience": 10}',
 'query': '// Initialize variables\nvar result = {\n    employees: [],\n    maxExperience: 0\n};\n\n// Loop through all employees\nfor (var i = 0; i < data.employees.length; i++) {\n    var emp = data.employees[i];\n    var projects = emp.profile && emp.profile.projects ? emp.profile.projects : [];\n    \n    // Loop through all projects\n    for (var j = 0; j < projects.length; j++) {\n        var tasks = projects[j].tasks ? projects[j].tasks : [];\n        \n        // Loop through all tasks\n        for (var k = 0; k < tasks.length; k++) {\n            var assignedTo = tasks[k].assignedTo;\n            if (assignedTo && assignedTo.skills && assignedTo.skills.experience) {\n                var expYears = assignedTo.skills.experience.years;\n                \n                if (expYears > result.maxExperience) {\n                    result.maxExperience = expYears;\n                    result.employees = [emp];\n                } else if (expYears === result.maxExperience) {\n                    // Check if employee already added\n                    var exists = result.employees.some(function(e) { return e.id === emp.id; });\n                    if (!exists) {\n                        result.employees.push(emp);\n                    }\n                }\n            }\n        }\n    }\n}',
 'modelAnswer': 'Based on the query results, the following employees have the **greatest experience (10 years)**:\n\n1. **Employee at index 5**\n2. **Employee at index 8**\n3. **Employee at index 9**\n4. **Employee at index 13**\n5. **Employee at index 18**\n6. **Employee at index 22**\n7. **Employee at index 34**\n8. **Employee at index 36**\n9. **Employee at index 37**\n10. **Employee at index 46**\n11. **Employee at index 48**\n12. **Employee at index 49**\n13. **Employee at index 52**\n14. **Employee at index 55**\n15. **Employee at index 67**\n16. **Employee at index 71**\n17. **Employee at index 86**\n18. **Employee at index 117**\n19. **Employee at index 122**\n20. **Employee at index 127**\n21. **Employee at index 134**\n22. **Employee at index 146**\n23. **Employee at index 158**\n24. **Employee at index 163**\n25. **Employee at index 167**\n26. **Employee at index 169**\n27. **Employee at index 177**\n28. **Employee at index 190**\n29. **Employee at index 199**\n\nThese employees all have **10 years of experience**, which is the highest in the dataset.\n\nWould you like their names or more details?'}
*/

TEST_CASE("Test aijsondb query issue 2", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 100;
	char buffer[nbuffer];
	std::string query = R"( 
// Initialize variables
var result = {
    employees: [],
    maxExperience: 0};
// Loop through all employees
for (var i = 0; i < data.employees.length; i++) {
    var emp = data.employees[i];
    var projects = emp.profile && emp.profile.projects ? emp.profile.projects : [];   
    // Loop through all projects
    for (var j = 0; j < projects.length; j++) {
        var tasks = projects[j].tasks ? projects[j].tasks : [];        
        // Loop through all tasks
        for (var k = 0; k < tasks.length; k++) {
            var assignedTo = tasks[k].assignedTo;
            if (assignedTo && assignedTo.skills && assignedTo.skills.experience) {
                var expYears = assignedTo.skills.experience.years;
                if (expYears > result.maxExperience) {
                    result.maxExperience = expYears;
                    result.employees = [emp];
                } else if (expYears === result.maxExperience) {
                    // Check if employee already added
                    var exists = result.employees.some(function(e) { return e.id === emp.id; });
                    if (!exists) {
                        result.employees.push(emp);
                    }
                }
            }
        }
    }
}
)";
	res = aijsondb_query(query.c_str(), buffer, nbuffer);
	REQUIRE(res == 0);
	std::string sres(buffer);
	try
	{
		jsoncons::json j = jsoncons::json::parse(sres);
		jsoncons::json je = j["employees"];
		REQUIRE(je.size() == 29);
		jsoncons::json jn = j["employees"][0]["name"];
		std::string name =	jn.to_string();
		REQUIRE(!name.empty());

	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
		FAIL("JSON parsing failed");
	}
}


TEST_CASE("Test aijsondb query issue 1 undefined", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V3.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V3.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 100;
	char buffer[nbuffer];
	std::string query = R"( 
var result = Date.ricol;
)";
	res = aijsondb_query(query.c_str(), buffer, nbuffer);
	REQUIRE(res == -1);
	std::string sres(buffer);
	REQUIRE(sres.size() > 0);
}

/*
int main(int argc, char* argv[]) {
	int res=domino_query_test();
	//int res = load_cache("C:/NHKI/dominodb/data/500 KB_V2.json");
	if(res!=0)
	{
		printf("Test failed with error code %d\n", res);
		return res;
	}
	else
	{
		printf("Test passed successfully\n");
		return 0;
	}
}
*/

TEST_CASE("Test aijsondb serialize", "[domino]") {

	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewContext(rt);
	{
		const char* eres = "\"a\"";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == "\"a\"");
		JS_FreeValue(ctx, jsv);
	}
	{
		const char* eres = "true";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == eres);
		JS_FreeValue(ctx, jsv);
	}
	{
		const char* eres = "[1,2,3]";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == eres);
		JS_FreeValue(ctx, jsv);
	}
	{
		const char* eres_test = "{\"a\":1}";
		const char* eres = "JSON.parse(\"{\\\"a\\\":1}\")";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == "{\"a\":1}");
		JS_FreeValue(ctx, jsv);
	}
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
}

TEST_CASE("Test aijsondb serialize complex", "[domino]") {

	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewContext(rt);
	{
		const char* eres_test = "{\"a\":1,\"b\":{\"b1\":1.5,\"b2\":\"2022-0511\"}}";
		const char* eres = "JSON.parse(\"{\\\"a\\\":1,\\\"b\\\":{\\\"b1\\\":1.5,\\\"b2\\\":\\\"2022-0511\\\"}}\")";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == eres_test);
		JS_FreeValue(ctx, jsv);
	}
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
}

TEST_CASE("Test aijsondb serialize datetime", "[domino]") {

	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewContext(rt);
	{
		const char* eresTest = "\"2026-04-11T08:30:00.000Z\"";
	    const char* eres = "new Date(Date.UTC(2026, 3, 11, 8, 30))";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(sst.str() == eresTest);
		JS_FreeValue(ctx, jsv);
		
	}
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
}

TEST_CASE("Test aijsondb serialize undefined", "[domino]") {

	JSRuntime* rt;
	JSContext* ctx;
	rt = JS_NewRuntime();
	ctx = JS_NewContext(rt);
	{
		const char* eres = "Date.nobel";
		JSValue jsv = JS_Eval(ctx, eres, strlen(eres), "<result>", JS_EVAL_TYPE_GLOBAL);
		if (JS_IsException(jsv)) {
			FAIL("Failed to evaluate JavaScript expression");
			JS_FreeValue(ctx, jsv);
			JS_FreeContext(ctx);
			JS_FreeRuntime(rt);
			return;
		}
		jsoncons::json j;
		toJsonWithVirtual(ctx, jsv, j);
		std::stringstream sst;
		sst << j;
		std::cout << "Serialized JSON: " << sst.str() << std::endl;
		REQUIRE(j.is_null());
		JS_FreeValue(ctx, jsv);

	}
	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
}

int test_mona();

/*
TEST_CASE("Test aijsondb read excel", "[domino]") {
    test_mona();
}
*/

#if false
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
		//printf("%s\n", aijsondb_data);

		init_functions(ctx);

		{
			std::string query = "aijsondb_buckets()";
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
			std::string query = "aijsondb_bucket_length('employees')";
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
				int32_t int_result = 0;
				int ih = JS_ToInt32(ctx, &int_result, jsv);
				printf("%d\n", int_result);
				JS_FreeValue(ctx, jsv);
			}
		}


		{
			std::string query = "aijsondb_bucket_object('employees',0)";
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
			std::string query = "aijsondb_schema()";
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
#endif