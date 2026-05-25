import ctypes
import json
import pprint
import platform
import os
import subprocess

def remove_if_exists(file):
    try:
        os.remove(file)
    except FileNotFoundError:
        pass

def query_data_javascript_qj():
    data = "C:/del/output_kita_utf8.json"
    remove_if_exists(data)
    schema="C:/del/output_kita_utf8_schema.json"
    remove_if_exists(schema)
    excel="C:/NHKI/data/talktodataexcel/kitaliste-nov-2025.xlsx"

    argument=f'-d "{data}" -s "{schema}" -i "{excel}" "var result=data.KitalisteVer_ffentlichung.length"';
    print('../build/release/aijsondbcli.exe '+argument)
    # Run a simple command
    result = subprocess.run('../build/release/aijsondbcli.exe '+ argument, capture_output=True, text=True)
    # Print the output
    print(result.stdout)
    #print("st")
   
def query_data_javascript_si():
    data = "../data/500 KB_V3.json"
    schema="../data/employeeSchemaDescription_V3.json"
    argument=f'-d "{data}" -s "{schema}" "var result=data.employees.length"'
    print('../build/release/aijsondbcli.exe '+argument)
    # Run a simple command
    result = subprocess.run('../build/release/aijsondbcli.exe '+ argument, capture_output=True, text=True)
    # Print the output
    print(result.stdout)

def query_data_javascript_imp():
    data = "C:/del/output_kita2_utf8.json"
    remove_if_exists(data)
    schema="C:/del/output_kita2_utf8_schema.json"
    remove_if_exists(schema)
    excel="C:/NHKI/data/talktodataexcel/kitaliste-nov-2025.xlsx"

    argument=f'-d "{data}" -s "{schema}" -i "{excel}" --import-only';
    print('../build/release/aijsondbcli.exe '+argument)
    # Run a simple command
    result = subprocess.run('../build/release/aijsondbcli.exe '+ argument, capture_output=True, text=True)
    # Print the output
    print(result.stdout)

    argument=f'-d "{data}" -s "{schema}" "var result=data.KitalisteVer_ffentlichung.length"'
    result = subprocess.run('../build/release/aijsondbcli.exe '+ argument, capture_output=True, text=True)
    # Print the output
    print(result.stdout)

    #print("st")

#query_data_javascript_qj()
#query_data_javascript_si()
query_data_javascript_imp()