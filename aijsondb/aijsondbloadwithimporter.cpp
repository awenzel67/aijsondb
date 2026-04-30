#include "aijsondbimporter.hpp"
#include "aijsondbloadwithimporter.h"
#include <map>
#include "aijsondblib.h"
#include <jsoncons/json.hpp>

int load_with_importer( IImporter* importer, const char* filepath,std::string& error)
{
	if (importer == nullptr)
	{
		error = "no importer";
		return -1;
	}
	auto obc = get_object_cache();
	obc->clear();

	if (!importer->init(filepath));
	{
		error = "File: ";
		error += filepath;
		error += " not loaded!";
		return -1;
	}


	std::vector<std::string> buckets = importer->buckets();
	if (buckets.size() == 0)
	{
		error = "No buckets!";
		return -1;
	}

	for (auto bucket : buckets)
	{
		std::string value;
		while (importer->next(value))
		{
			(*obc)[bucket].push_back(value);
		}
	}

	std::string* pschema = get_jobject_cache_schema();

	if (!importer->schema(*pschema))
	{
		error = "Schema could not be loaded.";
		return -1;
	}
	return 0;
}