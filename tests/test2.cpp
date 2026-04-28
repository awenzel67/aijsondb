#include <xlnt/xlnt.hpp>

#include <iostream>
#include <xlnt/xlnt.hpp>
#include <map>
#include <string>
#include <vector>
#include <jsoncons/json.hpp>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <set>
#include <chrono>

bool getHeaders(xlnt::worksheet& ws, std::map<int, std::vector<std::string>>& headers)
{
    //std::map<int, std::vector<std::string>> headers;
    std::clog << "Processing spread sheet" << std::endl;
    bool isHeaderLast = false;
    std::vector<std::string> headerRowLast;
    size_t irow = 0;
    for (auto row : ws.rows(false))
    {
        bool isHeader = false;
		std::vector<std::string> headerRow;
        int icollast = -1;
		int icolFirstEmpty = -1;
        for (auto cell : row)
        {
            int icell=cell.column_index();
            if (cell.data_type() == xlnt::cell_type::inline_string || cell.data_type() == xlnt::cell_type::shared_string)
            {
                //std::clog << cell.to_string() << std::endl;
                headerRow.push_back(cell.to_string());
				icollast = icell;
            }
            else if (cell.data_type() == xlnt::cell_type::empty)
            {
                if (icolFirstEmpty == -1) {
                    icolFirstEmpty = icell;
                }
			}
        }

		isHeader = icollast >= 0 && (icolFirstEmpty == -1 || icolFirstEmpty > icollast);

        if (isHeader)
        {
            if (isHeaderLast)
            {
                if(headerRowLast.size()<headerRow.size())
                {
                    headers[irow] = headerRow;
                    headerRowLast = headerRow;
                }
			}
            else
            { 
                headers[irow] = headerRow;
            }
        }
        isHeaderLast = isHeader;
        irow++;
    }
    std::clog << "Processing complete" << std::endl;
	return true;
}

bool getHeaders2(xlnt::worksheet& ws, std::map<int, std::vector<std::string>>& headers)
{
    //std::map<int, std::vector<std::string>> headers;
    std::clog << "Processing spread sheet" << std::endl;
    bool isHeaderLast = false;
    std::vector<std::string> headerRowMax;
    size_t irow = 0;
	size_t irowHeader=-1;
    for (auto row : ws.rows(false))
    {
        bool isHeader = false;
        std::vector<std::string> headerRow;
        int icollast = -1;
        int icolFirstEmpty = -1;
        for (auto cell : row)
        {
            int icell = cell.column_index();
            if (cell.data_type() == xlnt::cell_type::inline_string || cell.data_type() == xlnt::cell_type::shared_string)
            {
                //std::clog << cell.to_string() << std::endl;
                headerRow.push_back(cell.to_string());
                icollast = icell;
            }
            else if (cell.data_type() == xlnt::cell_type::empty)
            {
                if (icolFirstEmpty == -1) {
                    icolFirstEmpty = icell;
                }
            }
        }

        isHeader = icollast >= 0 && (icolFirstEmpty == -1 || icolFirstEmpty > icollast);

        if (isHeader)
        {
              if(headerRowMax.size()<headerRow.size())
              {
                  headerRowMax = headerRow;
                  irowHeader = irow;
			  }
        }
        irow++;
    }
    if (irowHeader != -1)
    {
        headers[irowHeader] = headerRowMax;
    }
    std::clog << "Processing complete" << std::endl;
    return true;
}


bool isHeaders(std::map<int, std::vector<std::string>>& headers,int irow)
{
     if (headers.find(irow) != headers.end())
     {
         return true;
	 }
     return false;
}

bool isBefore(std::map<int, std::vector<std::string>>& headers, int irow)
{
	int irow_min = -1;
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        if( irow_min <0 || it->first < irow_min)
        {
            irow_min = it->first;
		}
    }
    if (irow < irow_min)
    {
        return true;
    }
    return false;
}

std::vector<std::string>  getHeaderForRow(std::map<int, std::vector<std::string>>& headers, int irow)
{
	std::vector<std::string> empty;
    std::vector<int> headerRows;
    for (auto& header : headers)
    {
		headerRows.push_back(header.first);
	}
    
    std::sort(headerRows.begin(), headerRows.end());

    if(headerRows.size()==0)
    {
        return empty;
	}

    for (int i = headerRows.size()-1; i >= 0; i--)
    {
        if (irow >= headerRows[i])
        {
			int headerRow = headerRows[i];
			auto header = headers.find(headerRow);
            if (header != headers.end())
            {
                return header->second;
            }
            return empty;
        }
    }
    return empty;
}

std::wstring to_wstring(const std::string& utf8_str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    // Convert UTF-8 string to wide string
    std::wstring wide_str = converter.from_bytes(utf8_str);
    return wide_str;
}

std::string to_js_name_w(const std::wstring& label)
{
    std::set<wchar_t> forbiddenChar = {};
    std::string astr;
	bool firstAlpha = false;
    for (size_t i = 0; i < label.size(); i++)
    {
        wchar_t c = label[i];
        if (c < 172)
        {
            if (isalpha(c))
            {
                firstAlpha = true;
                char ac = static_cast<char>(c);
                astr += ac;
            }
            else if (isalnum(c))
            {
                if(firstAlpha)
                {
                    astr += static_cast<char>(c);
                }
            }
        }
        else
        {   
            if (firstAlpha) {
                astr += '_';
            }
        }
	}
   
    return astr;
}


std::string to_js_name(const std::string& label)
{
    std::wstring wjsname = to_wstring(label);
    return to_js_name_w(wjsname);
}

int test_mona()
{
    xlnt::workbook wb;
	std::string filename = "kitaliste-nov-2025.xlsx";
    auto start = std::chrono::high_resolution_clock::now();
    wb.load("C:/NHKI/data/talktodataexcel/" + filename);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken: " << duration.count()*1.0/1000.0/1000.0 << " microseconds" << std::endl;
    auto ws = wb.active_sheet();
    std::string sschema = R"(
    {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
            "$id" : "https://example.com/product.schema.json",
            "title" : "Product",
            "description" : "",
            "type" : "object",
            "properties": {}
    }
    )";

	jsoncons::json js=jsoncons::json::parse(sschema);
	filename.replace(filename.find(".xlsx"),5, "");
    auto jsfilename = to_js_name(filename);
	js["title"] = filename;
    std::string sheetlabel = ws.title();
    std::string sheetname = to_js_name(sheetlabel);
    js["properties"][sheetname] = jsoncons::json::object();
	js["properties"][sheetname]["type"] = "array";
    js["properties"][sheetname]["description"] = "Array of: "+sheetlabel;
    js["properties"][sheetname]["items"] = jsoncons::json::object();
    js["properties"][sheetname]["items"]["type"]="object";
    js["properties"][sheetname]["items"]["description"] = sheetlabel;
    js["properties"][sheetname]["items"]["properties"]= jsoncons::json::object();
    auto jsArrayRow = jsoncons::json::object();
    std::map<int, std::vector<std::string>> headers;
	getHeaders2(ws, headers);
	int irow = 0;
    jsoncons::json jsbucket = jsoncons::json::object();
	jsoncons::json j(jsoncons::json_array_arg);
    std::clog << "Processing spread sheet" << std::endl;
	bool schemaCreated = false;
    for (auto row : ws.rows(false))
    {
        if (isBefore(headers, irow))
        {
            irow++;
            continue;
        }

		bool isHeader = isHeaders(headers, irow);
        if (isHeader)
        {
            if (!schemaCreated) {
                std::vector<std::string> headerRow = getHeaderForRow(headers, irow);
                for (size_t i = 0; i < headerRow.size(); i++)
                {
                    std::string headerLabel = headerRow[i];
                    std::string header = to_js_name(headerRow[i]);
                    js["properties"][sheetname]["items"]["properties"][header] = jsoncons::json::object();
                    js["properties"][sheetname]["items"]["properties"][header]["type"] = "string";
                    js["properties"][sheetname]["items"]["properties"][header]["description"] = headerLabel;
                }
                schemaCreated = true;
            }
            irow++;
            continue;
		}
		std::vector<std::string> headerRow = getHeaderForRow(headers,irow);
		jsoncons::json jrow;
        for (auto cell : row)
        {
            int icell=cell.column_index();
            if (icell - 1 < headerRow.size())
            {
                std::string headerLabel = headerRow[icell - 1];
                std::string header = to_js_name(headerLabel);
                jrow[header] = cell.to_string();
            }
            //std::clog << cell.to_string() << std::endl;
        }
		j.push_back(jrow);
		irow++;
        //if (irow > 100)
        //{
        //    break;
        //}
    }
    
    //std::clog << j.to_string() << std::endl;
   

    // Optional: Write UTF-8 BOM (Byte Order Mark) if needed for Windows apps like Notepad
    // unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    // outFile.write(reinterpret_cast<char*>(bom), sizeof(bom));

    // Write UTF-8 text
   
    {
        jsbucket[sheetname] = j;
        std::ofstream outFile("C:/del/output_utf8.json", std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("Failed to open file for writing.");
        }
        std::string utf8Text = jsbucket.to_string();
        outFile.write(utf8Text.c_str(), utf8Text.size());
    }
    {
        std::ofstream outFile("C:/del/output_utf8_schema.json", std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("Failed to open file for writing.");
        }
        std::string utf8Text = js.to_string();
        outFile.write(utf8Text.c_str(), utf8Text.size());
    }
    std::clog << "Processing complete" << std::endl;
    return 0;
}