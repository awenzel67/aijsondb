#include "aijsondblib.h"
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
}