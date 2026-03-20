/* File generated automatically by the QuickJS-ng compiler. */

#include "aijsondblib.h"
#include <stdio.h>
#include "../third_party/cli11/CLI11.hpp"

int main(int argc, char **argv)
{
    CLI::App app{ "query json data conforming to a schema, using JavaScript. The query is executed in a secure sandbox." };
    argv = app.ensure_utf8(argv);

    std::string database = "";
    app.add_option("-d,--database", database, "path to json data file");
	std::string schema = "";    
    app.add_option("-s,--schema", schema, "path to json schema file");
    std::string query = "";
    app.add_option("query", query, "query to execute (in quotes, e.g. \"result = employee.length\"");
    CLI11_PARSE(app, argc, argv);
    
    if (database.empty())
    {
        printf("Please provide a data file path using -d or --database option.\n");
        return 1;
    }
    if(schema.empty())
    {
        printf("Please provide a schema file path using -s or --schema option.\n");
        return 1;
	}

    if(query.empty())
    {
        printf("Please provide a query to execute.\n");
        return 1;
	}

 
  int iload=aijsondb_load_data(database.c_str(), schema.c_str());
  if(iload!=0)
  {
    printf("Error loading database or schema.\n");
    return 1;
  }

  const int nbuffer = 1024*10;
  char buffer[nbuffer];
  aijsondb_query(query.c_str(), buffer, nbuffer);
  printf("%s", buffer);
  return 0;
}

/*
./dominodbcli.exe -d "../../data/500 KB_V2.json" -s "../../data/employeeSchemaDescription_V2.json" "var result=data.employees.length;"
*/