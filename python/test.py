import ctypes
import json
import pprint
import platform
def query_data_javascript_qj():

    libObject = None;
    system = platform.system()
    if system == "Windows":
      libObject=ctypes.WinDLL('../build/debug/aijsondbc.dll')
    elif system=="Linux":
      libObject=ctypes.CDLL('../build/libaijsondbc.so')

    cdata = ctypes.c_char_p(b"../data/500 KB_V2.json")
    cschema=ctypes.c_char_p(b"../data/employeeSchemaDescription_V2.json")

    with open("query.txt", "r", encoding="utf-8") as file:
        content = file.read()
    
    bquery=content.encode()

    cquery = ctypes.c_char_p(bquery)
    is_schema=libObject.ffi_aijsondb_load_data(cdata,cschema)
    nbuffer= ctypes.c_int32=1024*1000
    buffer = ctypes.create_string_buffer(nbuffer)
    if is_schema == 0:
        print("Data loaded successfully")
    else:
        libObject.ffi_aijsondb_last_error(buffer,nbuffer)
        serror=buffer.value.decode()
        print(serror)
        raise Exception(serror)
    
    res=libObject.ffi_aijsondb_query(cquery,buffer,nbuffer)
    if res == 0:
        sjson=buffer.value.decode()
        ret=json.loads(sjson)
        print("Query with qjs")
        pprint.pprint(ret)
    else:
        serror=buffer.value.decode()
        print(serror)
        raise Exception(serror)

query_data_javascript_qj()