#ifndef AIJSONDB_IMPORTER_H
#define AIJSONDB_IMPORTER_H
#include <string>
#include <vector>
#include <map>
class IImporter
{
public:
    virtual bool init(const std::string& filepath) = 0;
    virtual std::vector<std::string> buckets() = 0;
    virtual bool next(std::string& value) = 0;
    virtual bool schema(std::string& schema) = 0;
    virtual const char* ending() = 0;
    virtual ~IImporter() {}
};

class IBulkImporter
{
public:
    virtual bool import(const std::string& filepath, std::map<std::string,std::vector<std::string>>& cache, std::string& schema, std::string& error) = 0;
    virtual const char* ending() = 0;
    virtual ~IBulkImporter() {}
};

#endif