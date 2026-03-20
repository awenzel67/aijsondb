#ifndef DOMINODBLIB_H
#define DOMINODBLIB_H
int aijsondb_load_data(const char* filepath_data, const char* filepath_schema);
int aijsondb_query(const char* query, char* buffer, int nbuffer);
int aijsondb_free_data();
const char* aijsondb_last_error();
int aijsondb_query_test();
int load_cache(const char* filepath);
#endif