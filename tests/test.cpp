#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include "../aijsondb/aijsondblib.h"
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

const char* test_data_dir()
{
	return TEST_DATA_DIR;
}

TEST_CASE("Test Domino Load", "[domino]") {
	std::string path_data=test_data_dir();
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	int res = aijsondb_load_data(path_data.c_str(),path_schema.c_str());
    REQUIRE(res==0);
	REQUIRE(strlen(aijsondb_last_error()) == 0);
}


const char* query_test1 = "var result = null;\n\n// Iterate through each employee\nfor (var i = 0; i < data.employees.length; i++) {\n    var employee = data.employees[i];\n    var projects = employee.profile.projects || [];\n    \n    // Iterate through each project of the employee\n    for (var j = 0; j < projects.length; j++) {\n        var project = projects[j];\n        \n        // Check if the project name matches\n        if (project.name === \"Incubate World-Class Schemas\") {\n            result = employee;\n            break;\n        }\n    }\n    \n    if (result !== null) {\n        break;\n    }\n};result";
const char* query_test2 = "var result = data.employees[101];result";

int aijsondb_query(const char* query, char* buffer, int nbuffer);

TEST_CASE("Test Domino query bucket object", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	char buffer[1024];
	res = aijsondb_query(query_test1,buffer,1024);
	std::cout << "Query result: " << buffer << std::endl;

	REQUIRE(res == 0);
}


const char* query_test3 = "var result = data.employees;result";
TEST_CASE("Test Domino query bucket array", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	const int nbuffer = 1024 * 1000;
	char* buffer= new char[nbuffer];
	res = aijsondb_query(query_test3, buffer,nbuffer);
	std::cout << "Query result: " << buffer << std::endl;
	delete buffer;
	REQUIRE(res == 0);
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
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
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

const char* TEST_QUERY_KO4 = "\nvar result = data.employees.filter(\nemployee => {\n  return employee.profile.projects.some(\n     project => {\n         return  project.name == 'Incubate World-Class Schema'; \n     } \n) \n}\n).map(employee => ({ \n    id : employee.id,\n    name : employee.name\n })\n)[0]    \n";

TEST_CASE("Test problem with query 1", "[domino]") {
	std::string path_data = test_data_dir();
	path_data += "500 KB_V2.json";
	std::string path_schema = test_data_dir();
	path_schema += "employeeSchemaDescription_V2.json";
	int res = aijsondb_load_data(path_data.c_str(), path_schema.c_str());
	REQUIRE(res == 0);
	char buffer[1024];
	std::string query = TEST_QUERY_KO4;
	res = aijsondb_query(query.c_str(), buffer, 1024*1000);
	std::cout << "Query result: " << buffer << std::endl;
	std::string sres(buffer);
	REQUIRE(res == 0);
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