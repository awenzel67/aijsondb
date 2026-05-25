import ctypes
import json
import pprint
import platform
import os

def remove_if_exists(file):
    try:
        os.remove(file)
    except FileNotFoundError:
        pass

def query_data_javascript_qj():

    libObject = None;
    system = platform.system()
    if system == "Windows":
      libObject=ctypes.WinDLL('../build/release/aijsondbc.dll')
    elif system=="Linux":
      libObject=ctypes.CDLL('../build/libaijsondbc.so')

    remove_if_exists("C:/del/output_kita_utf8.json")
    cdata = ctypes.c_char_p(b"C:/del/output_kita_utf8.json")
    remove_if_exists("C:/del/output_kita_utf8_schema.json")
    cschema=ctypes.c_char_p(b"C:/del/output_kita_utf8_schema.json")
    cexcel=ctypes.c_char_p(b"C:/NHKI/data/talktodataexcel/kitaliste-nov-2025.xlsx")

    with open("query_kita.txt", "r", encoding="utf-8") as file:
        content = file.read()
    
    bquery=content.encode()

    cquery = ctypes.c_char_p(bquery)
    #ffi_aijsondb_import_or_load_data
    is_schema=libObject.ffi_aijsondb_import_or_load_data(cexcel,cdata,cschema)
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
        print(sjson)
        ret=json.loads(sjson)
        print("Query with qjs")
        pprint.pprint(ret)
    else:
        serror=buffer.value.decode()
        print(serror)
        raise Exception(serror)

query_data_javascript_qj()