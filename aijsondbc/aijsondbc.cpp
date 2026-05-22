#include "aijsondblib.h"
#include "../aijsondbexcel/aijsondbimportexcel.h"
// mylib.h
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#include <string.h>

extern "C" {
   EXPORT	int ffi_aijsondb_load_data(const char* filename, const char* schema) {
	   return aijsondb_load_data(filename, schema);
	}

   EXPORT	int ffi_aijsondb_import_or_load_data(const char* filename, const char* json_filename, const char* schema) {
	   IBulkImporter* o = new ExcelImporter();
	   std::unique_ptr<IBulkImporter> op(o);
	   register_importer(op);
	   return aijsondb_load_data_with_cache(filename,json_filename, schema);
   }
	EXPORT  int ffi_aijsondb_query(const char* query, char* result_buffer, int buffer_size) {
		return aijsondb_query(query, result_buffer, buffer_size);
	}
	EXPORT int ffi_aijsondb_free_data() {
		return aijsondb_free_data();
	}
	EXPORT  int ffi_aijsondb_last_error(char* result_buffer, int buffer_size) {
		if (buffer_size < 1)
			return -1;
		result_buffer[0] = '\0';
		const char* last_error = aijsondb_last_error();
		if (last_error == nullptr)
			return 0;

		if (strlen(last_error) > buffer_size - 1)
			return -1;
	    strcpy(result_buffer, last_error);
		return 0;
	}
	EXPORT int ffi_aijsondb_query_result_set(const char* query) {
		return aijsondb_query_result_set(query);
	}
	EXPORT int ffi_aijsondb_result_set_next(int index_result_set, int index_next, char* bucket, int nbucket, char* buffer, int nbuffer, int* isArray)
	{
		return aijsondb_result_set_next(index_result_set, index_next, bucket, nbucket, buffer, nbuffer, isArray);
	}
	EXPORT int ffi_aijsondb_result_set_clear(int index_result_set) {
		return aijsondb_result_set_clear(index_result_set);
	}
}