# dominodb - loved by AIs

Like any other database dominodb can be used to query a datastructure to get an answer data structure as result. What make it different is that we chose:

* A single JSON document as its data structure.
* JavaScript as the query language.
* JSON Schema to describe the JSON document.

These features are not good choices if a human writes the query but are rather good choices if an actual llm creates the query based on user input in natural language.

## Getting started

### Prerequisites

dominodb uses [CMake](https://cmake.org/) as its main build system. 

### Installation

#### Linux

Clone the repository:
```
git clone https://github.com/awenzel67/dominodb.git
```
Change working directory:
```
cd dominodb
```

Build the project: 
```
cmake -B build
cmake --build build
```

Run the cli interface:
```
cd build
dominodbcli --database "../data/500 KB_V2.json" --schema "../data/employeeSchemaDescription_V2.json" "var result=data.employees.length"
```

#### Windows

Open the Visual Studio Developer Console.

Clone the repository:
```
git clone https://github.com/awenzel67/dominodb.git
```
Change working directory:
```
cd dominodb
```

Build the project: 
```
cmake -B build
cmake --build build
```

Run the cli interface:
```
cd build/Release
dominodbcli --database "../../data/500 KB_V2.json" --schema "../../data/employeeSchemaDescription_V2.json" "var result=data.employees.length"
```

## cli

The build process creates the executable: 
* Windows: dominodbcli.exe
* Linux: dominodbcli

It can be used to query a json data object in a file.
```
dominodbcli --database --schema "query"
```
--database path to the file containing the json object.\
--schema path to the file containing the schema.\
--"query" javascript snippet to query the json object. By convention the result is saved in variable result.

The output on the terminal shows the result as json.

To query the name of all employees use the following command:

```
dominodbcli --database "../../data/500 KB_V2.json" --schema "../../data/employeeSchemaDescription_V2.json" "var result=data.employees.map(x=>x.name)"
```


## C API

During the build process additionally to the cli a dynamic library is created containing a simple API with the c functions:

```
int ffi_domino_load_data(const char* filename, const char* schema)
```
* parameters:
    * const char* filename: path to the file containing the json object,
    * const char* schema: path to the file containing the schema.
* return value: 0 success, -1 error. 

This function loads the json data from file into the C++ in memory data structure. It also validates the data using the schema file.


```
int ffi_domino_query(const char* query, char* result_buffer, int buffer_size)
```
* parameters:
  * const char* query: javascript expression to query the json data,
  * char* result_buffer: result json string buffer, 
  * int buffer_size: maximum length of json string buffer.
* return value: 0 success, -1 error. 

The function executes the query on the json data loaded before. The result is a json string which can be found in the result buffer. If an error occurs the return value is -1 and the result_buffer contains an error message.

```
int ffi_domino_last_error(char* result_buffer, int buffer_size)
```
* parameters:
  * char* result_buffer: last error message, 
  * int buffer_size: maximum length of buffer.
* return value: 0 success, -1 error. 

This function can be used to get the actual errormessage if a function returns an error.

```
int ffi_domino_free_data()
```
Unload the json data and schema.

The resulting dynamic library can be used from other programming languages like Python, Java or C#.

A Python sample can be found in python folder.

## Technology

### Data structure

The json file is parsed into a C++ in memory data structure consisting of buckets containing arrays of json data.

Handling the json data is done using the [jsoncons](https://github.com/danielaparker/jsoncons) library. 

### Query Engine

dominodb uses the [quickjs-ng](https://github.com/quickjs-ng/quickjs) javascript engine to apply the query on the json dataobject.
Javascript data objects are loaded on demand from the C++ in memory data structure.

### Schema 

The json file is validated loading the data into the in memory data structure. 
Library [jsoncons](https://github.com/danielaparker/jsoncons) is used for this reason. 

### Others

Testing: [Catch2](https://github.com/catchorg/Catch2) 

CLI: [CLI11](https://github.com/CLIUtils/CLI11)

## Status

dominodb is actually used for comparing json/jsonschema/javascript with sqldatabase/sql/sqlschema. 
It works well for this purpose and gives good results. On the other hand, the datasets used (see JSON files in the data directory) are rather small, and this is an alpha version, not production ready.

Contributors interested in working with me on a database loved by AIs are welcome.