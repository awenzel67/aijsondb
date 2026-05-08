#ifndef DOMINODBLIB_H
#define DOMINODBLIB_H
#include "aijsondbimporter.hpp"
#include <memory>
int aijsondb_load_data(const char* filepath_data, const char* filepath_schema);
int aijsondb_save_data(const char* filepath_data);
int aijsondb_query(const char* query, char* buffer, int nbuffer);
int aijsondb_free_data();
const char* aijsondb_last_error();
int load_cache(const char* filepath);
const char* get_bucket_object(const char* name,int index);
const char* get_bucket_name_from_index(int index);
bool register_importer(std::unique_ptr<IBulkImporter>& importer);
void clear_importer(std::unique_ptr<IBulkImporter>& importer);
IBulkImporter* get_importer(const std::string& ending);
int aijsondb_load_data_with_cache(
	const char* filename,
	const char* json_filename,
	const char* schema
);
std::map<std::string, std::vector<std::string>>* get_object_cache();
std::string* get_jobject_cache_schema();
#endif