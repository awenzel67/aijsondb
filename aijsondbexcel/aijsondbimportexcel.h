#ifndef AIJSONDB_EXCEL_IMPORT_H
#define AIJSONDB_EXCEL_IMPORT_H
#include "../aijsondb/aijsondbimporter.hpp"
#include <map>

class ExcelImporter: public IBulkImporter
{
public:
    ExcelImporter();
    virtual bool import(const std::string& filepath, std::map<std::string, std::vector<std::string>>& cache, std::string& schema, std::string& error);
    virtual const char* ending() { return ".xlsx"; }
    virtual ~ExcelImporter() {}
};
#endif
