# aijsondb – Query Your Data with JavaScript

**Loved by AIs: Now with Excel importer!**

The idea behind **aijsondb** is inspired by **[DuckDB](https://github.com/duckdb/duckdb)**. It allows you to query your data and receive a result dataset as output. What sets it apart is our choice of:

- A **single JSON document** as the data structure.
- **JavaScript** as the query language.
- **JSON Schema** to describe the JSON document.

While these choices may not be ideal for humans writing queries, they are well-suited for **LLMs** generating queries based on natural language input.

In a comparison of an **aijsondb**-based JavaScript agent and an SQL agent using the same 100 questions on the same dataset, **aijsondb achieved 95% correct answers**, compared to 81% for SQL. For more details, see [this article](https://medium.com/@awenzel67/talk-to-your-data-a-new-approach-using-javascript-instead-of-sql-yields-better-results-d0c3d260cb68).

## Features

**aijsondb** is designed for use with AI agents. Library bindings are available for:

- [Python](https://github.com/awenzel67/aijsondb-py)
- [Java](https://github.com/awenzel67/aijsondb-java)

Additionally, **aijsondb** includes a CLI for:

- Validating your JSON data against a JSON Schema.
- Querying data with JavaScript.
- Importing data from XLSX (Excel).
- Creating JSON data and JSON Schema from XLSX (Excel).

## Getting started

### Prerequisites

aijsondb uses [CMake](https://cmake.org/) as its main build system. 

### Installation

#### Linux

Clone the repository:
```
git clone https://github.com/awenzel67/aijsondb.git
```
Change working directory:
```
cd aijsondb
```

Build the project: 
```
cmake -B build
cmake --build build
```

Run the cli interface:
```
cd build
aijsondbcli --database "../data/500 KB_V2.json" --schema "../data/employeeSchemaDescription_V2.json" "var result=data.employees.length"
```

#### Windows

Open the Visual Studio Developer Console.

Clone the repository:
```
git clone https://github.com/awenzel67/aijsondb.git
```
Change working directory:
```
cd aijsondb
```

Build the project: 
```
cmake -B build
cmake --build build --config Release
```

Run the cli interface:
```
cd build/Release
./aijsondbcli.exe --database "../../data/500 KB_V2.json" --schema "../../data/employeeSchemaDescription_V2.json" "var result=data.employees.length"
```

## cli

The build process creates the executable: 
* Windows: aijsondbcli.exe
* Linux: aijsondbcli

It can be used to query a json data object in a file.
```
aijsondbcli --database --schema "query"
```
--database path to the file containing the json object.\
--schema path to the file containing the schema.\
--"query" javascript snippet to query the json object. By convention the result is saved in variable result.

The output on the terminal shows the result as json.

To query the name of all employees use the following command:

```
aijsondbcli --database "../../data/500 KB_V2.json" --schema "../../data/employeeSchemaDescription_V2.json" "var result=data.employees.map(x=>x.name)"
```


## C API

During the build process additionally to the cli a dynamic library is created containing a simple API with the c functions:

```
int ffi_aijsondb_load_data(const char* filename, const char* schema)
```
* parameters:
    * const char* filename: path to the file containing the json object,
    * const char* schema: path to the file containing the schema.
* return value: 0 success, -1 error. 

This function loads the json data from file into the C++ in memory data structure. It also validates the data using the schema file.


```
int ffi_aijsondb_query(const char* query, char* result_buffer, int buffer_size)
```
* parameters:
  * const char* query: javascript expression to query the json data,
  * char* result_buffer: result json string buffer, 
  * int buffer_size: maximum length of json string buffer.
* return value: 0 success, -1 error. 

The function executes the query on the json data loaded before. The result is a json string which can be found in the result buffer. If an error occurs the return value is -1 and the result_buffer contains an error message.

```
int ffi_aijsondb_last_error(char* result_buffer, int buffer_size)
```
* parameters:
  * char* result_buffer: last error message, 
  * int buffer_size: maximum length of buffer.
* return value: 0 success, -1 error. 

This function can be used to get the actual errormessage if a function returns an error.

```
int ffi_aijsondb_free_data()
```
Unload the json data and schema.

The resulting dynamic library can be used from other programming languages like Python, Java or C#.

A Python sample can be found in python folder.

## Technology

### Data structure

The json file is parsed into a C++ in memory data structure consisting of buckets containing arrays of json data.

Handling the json data is done using the [jsoncons](https://github.com/danielaparker/jsoncons) library. 

### Query Engine

aijsondb uses the [quickjs-ng](https://github.com/quickjs-ng/quickjs) javascript engine to apply the query on the json dataobject.
Javascript data objects are loaded on demand from the C++ in memory data structure.

### Schema 

The json file is validated loading the data into the in memory data structure. 
Library [jsoncons](https://github.com/danielaparker/jsoncons) is used for this reason.

### Others

Testing: [Catch2](https://github.com/catchorg/Catch2) 

CLI: [CLI11](https://github.com/CLIUtils/CLI11)

## Status

aijsondb is actually used for comparing json/jsonschema/javascript with sqldatabase/sql/sqlschema. 
It works well for this purpose and gives good results. On the other hand, the datasets used (see JSON files in the data directory) are rather small, and this is an alpha version, not production ready.

Contributors interested in working with me on a database loved by AIs are welcome.

## Sample json data

The file 500_KB_V2.json in the data folder is derived from the dataset "Employees { 10 } Level Nested Formatted Versions", specifically the "500 KB 10 Level Formatted" variant available at [page](https://sample.json-format.com/).

The files test.json and 500_KB_V2Err.json are also based on this dataset.

The JSON schema employeeSchemaDescription_V2.json was automatically generated from 500_KB_V2.json and subsequently edited by hand.