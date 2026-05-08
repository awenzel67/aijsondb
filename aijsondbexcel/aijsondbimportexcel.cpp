#include <OpenXLSX.hpp>
#include "aijsondbimportexcel.h"
#include <iostream>
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
#include <filesystem>
using namespace std;
using namespace OpenXLSX;

E_EXCEL_LOGICAL_TYPE get_logical_type(XLDocument& doc,OpenXLSX::XLCell& cell)
{
    auto cellValueType = cell.value().type();
    if (cellValueType == OpenXLSX::XLValueType::Empty)
    {
        return E_EXCEL_LOGICAL_TYPE::EMPTY;
    }
    if (cellValueType == OpenXLSX::XLValueType::Boolean)
    {
        return E_EXCEL_LOGICAL_TYPE::BOOLEAN;
    }
   // if (cellValueType == OpenXLSX::XLValueType::Integer)
   // {
   //     return E_EXCEL_LOGICAL_TYPE::INTEGER;
   // }
    if (cellValueType == OpenXLSX::XLValueType::String)
    {
        return E_EXCEL_LOGICAL_TYPE::STRING;
    }
    if (cellValueType == OpenXLSX::XLValueType::Error)
    {
        return E_EXCEL_LOGICAL_TYPE::ERROR;
    }
    if (cellValueType != OpenXLSX::XLValueType::Float && cellValueType != OpenXLSX::XLValueType::Integer)
    {
        return E_EXCEL_LOGICAL_TYPE::EMPTY;
    }


    try {

        auto format = cell.cellFormat();
        auto mystyles = doc.styles();
        auto ff = mystyles.cellFormats()[format];
        auto nf = ff.numberFormatId();
        auto mynf = mystyles.numberFormats();
        auto sfm = mynf.numberFormatById(nf);
        auto code = sfm.formatCode();
        bool isTime = (code.find("hh") != std::string::npos) || (code.find("HH") != std::string::npos) || (code.find("h") != std::string::npos) || (code.find("H") != std::string::npos);
        bool isDate = (code.find("dd") != std::string::npos) || (code.find("DD") != std::string::npos) || (code.find("yy") != std::string::npos) || (code.find("yy") != std::string::npos);
        bool isInteger = cellValueType == OpenXLSX::XLValueType::Integer;

        if (isInteger)
        {
            if (isTime && isDate)
                return  E_EXCEL_LOGICAL_TYPE::DATE;
            else if (isDate)
                return  E_EXCEL_LOGICAL_TYPE::DATE;
            else if (isTime)
                return  E_EXCEL_LOGICAL_TYPE::TIME;
            else
                return E_EXCEL_LOGICAL_TYPE::INTEGER;
        }
        else
        {
            if (isTime && isDate)
                return  E_EXCEL_LOGICAL_TYPE::DATE_TIME;
            else if (isTime)
                return  E_EXCEL_LOGICAL_TYPE::TIME;
            else if (isDate)
                return  E_EXCEL_LOGICAL_TYPE::DATE;
            else
                return E_EXCEL_LOGICAL_TYPE::DOUBLE;
        }
    }
    catch (std::exception& ex)
    {
        return  E_EXCEL_LOGICAL_TYPE::ERROR;
    }
}

void add_cell_type(std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols, size_t irow, size_t icol, E_EXCEL_LOGICAL_TYPE celltype)
{
    for (size_t icolr = typecols.size(); icolr <= icol; icolr++)
    {
        typecols.push_back(std::vector<E_EXCEL_LOGICAL_TYPE>());
    }
    for (size_t icolr=0; icolr<typecols.size();icolr++)
    {
        size_t size_col = typecols[icolr].size();
        for (size_t irowr = size_col; irowr <= irow; irowr++)
        {
            typecols[icolr].push_back(E_EXCEL_LOGICAL_TYPE::EMPTY);
        }
    }
    typecols[icol][irow] = celltype;
}

E_EXCEL_LOGICAL_TYPE get_cell_type(std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols, size_t irow_header, size_t icol)
{
    if (icol >= typecols.size())
        return E_EXCEL_LOGICAL_TYPE::EMPTY;
    if (irow_header >= typecols[icol].size())
        return E_EXCEL_LOGICAL_TYPE::EMPTY;

    std::map < E_EXCEL_LOGICAL_TYPE, size_t> typecounts;
    for (size_t irowr = irow_header + 1; irowr < typecols[icol].size(); irowr++)
    {
        auto hkl = typecounts.find(typecols[icol][irowr]);
        if (hkl == typecounts.end()) {
            typecounts[typecols[icol][irowr]] = 1;
        }
        else
        {
            (*hkl).second = (*hkl).second + 1;
        }
    }

    E_EXCEL_LOGICAL_TYPE valmax = E_EXCEL_LOGICAL_TYPE::EMPTY;
    size_t count_max=0;

	bool hasDateTomeAndDate = 
        typecounts.find(E_EXCEL_LOGICAL_TYPE::DATE_TIME) != typecounts.end()
		&& typecounts.find(E_EXCEL_LOGICAL_TYPE::DATE) != typecounts.end()
        ;

    if (hasDateTomeAndDate)
    {
        typecounts[E_EXCEL_LOGICAL_TYPE::DATE_TIME]= typecounts[E_EXCEL_LOGICAL_TYPE::DATE_TIME]+ typecounts[E_EXCEL_LOGICAL_TYPE::DATE];
        typecounts[E_EXCEL_LOGICAL_TYPE::DATE] = 0;
    }

    bool hasIntegerAndDouble =
        typecounts.find(E_EXCEL_LOGICAL_TYPE::DOUBLE) != typecounts.end()
        && typecounts.find(E_EXCEL_LOGICAL_TYPE::INTEGER) != typecounts.end()
	;

    if (hasIntegerAndDouble)
    {
        typecounts[E_EXCEL_LOGICAL_TYPE::DOUBLE] = typecounts[E_EXCEL_LOGICAL_TYPE::DOUBLE] + typecounts[E_EXCEL_LOGICAL_TYPE::INTEGER];
        typecounts[E_EXCEL_LOGICAL_TYPE::INTEGER] = 0;
	}

    for (auto typecount : typecounts)
    {
        if ( typecount.first != E_EXCEL_LOGICAL_TYPE::EMPTY && typecount.second > count_max)
        {
            valmax = typecount.first;
            count_max = typecount.second;
        }
    }

    return valmax;
}


bool getHeaders2(XLDocument& doc, XLWorksheet& ws, std::map<int, std::vector<std::string>>& headers, std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols)
{
    //std::map<int, std::vector<std::string>> headers;
    std::clog << "Processing spread sheet" << std::endl;
    bool isHeaderLast = false;
    std::vector<std::string> headerRowMax;
    size_t irow = 0;
    size_t irowHeader = -1;
    typecols.clear();
    //std::vector<std::vector<int>> typecols;
    for (auto& row : ws.rows())
    {
        bool isHeader = false;
        std::vector<std::string> headerRow;
        int icollast = -1;
        int icolFirstEmpty = -1;
        int icell = 0;
		bool hasNonHeadrCells = false;
        for (auto& ccell : row.cells())
        {

            // durch diese Zeilen:
            const auto& cell = ccell.value();
            
             // XLCellValueProxy ist implizit konvertierbar zu XLCellValue
            if (cell.type() == OpenXLSX::XLValueType::String)
            {
                //std::clog << cell.to_string() << std::endl;
                headerRow.push_back(cell.getString());
                icollast = icell;
            }
            else if (cell.type() == OpenXLSX::XLValueType::Empty)
            {
                if (icolFirstEmpty == -1) {
                    icolFirstEmpty = icell;
                }
            }
            else
            { 
				hasNonHeadrCells = true;
            }
            auto logical_type = get_logical_type(doc, ccell);
            add_cell_type(typecols, irow, icell, logical_type);
            icell++;
        }

        isHeader = icollast >= 0 && (icolFirstEmpty == -1 || icolFirstEmpty > icollast) && !hasNonHeadrCells;

        if (isHeader)
        {
            if (headerRowMax.size() < headerRow.size())
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

bool isHeaders(std::map<int, std::vector<std::string>>& headers, int irow)
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
        if (irow_min < 0 || it->first < irow_min)
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

    if (headerRows.size() == 0)
    {
        return empty;
    }

    for (int i = headerRows.size() - 1; i >= 0; i--)
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
                if (firstAlpha)
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



bool ExcelImporter::import(const std::string& filepath, std::map<std::string, std::vector<std::string>>& cache, std::string& schema,std::string& error)
{
    cache.clear();
    schema.clear();
    XLDocument doc;
    //doc.create("./Demo01.xlsx", XLForceOverwrite);
    std::filesystem::path apa(filepath);
    std::string filename = apa.filename().u8string();
    auto start = std::chrono::high_resolution_clock::now();
    doc.open(filepath);
   
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

    jsoncons::json js = jsoncons::json::parse(sschema);
    filename.replace(filename.find(".xlsx"), 5, "");
    auto jsfilename = to_js_name(filename);
    js["title"] = filename;

    auto nsheets = doc.workbook().sheetCount();
    for (size_t iws = 0; iws < nsheets; iws++)
    {
        auto ws = doc.workbook().worksheet(iws+1);
        std::string sheetlabel = ws.name();
        std::string sheetname = to_js_name(sheetlabel);
        auto jsArrayRow = jsoncons::json::object();
        std::map<int, std::vector<std::string>> headers;
        std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>> typecols;
        getHeaders2(doc,ws, headers,typecols);
        if (headers.size() == 0)
        {
            continue;
		}
        js["properties"][sheetname] = jsoncons::json::object();
        js["properties"][sheetname]["type"] = "array";
        js["properties"][sheetname]["description"] = "Array of: " + sheetlabel;
        js["properties"][sheetname]["items"] = jsoncons::json::object();
        js["properties"][sheetname]["items"]["type"] = "object";
        js["properties"][sheetname]["items"]["description"] = sheetlabel;
        js["properties"][sheetname]["items"]["properties"] = jsoncons::json::object();
        int irow = 0;
        //jsoncons::json jsbucket = jsoncons::json::object();
        //jsoncons::json j(jsoncons::json_array_arg);
        std::clog << "Processing spread sheet" << std::endl;
        bool schemaCreated = false;
        std::vector<E_EXCEL_LOGICAL_TYPE> target_types;
        std::vector<string> values;
        for (auto& row : ws.rows())
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
                        js["properties"][sheetname]["items"]["properties"][header]["description"] = headerLabel;
                        auto celltype = get_cell_type(typecols, irow, i );
                        switch (celltype)
                        {
                        case E_EXCEL_LOGICAL_TYPE::STRING :  
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "string";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::BOOLEAN:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "boolean";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::DOUBLE:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "number";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::INTEGER:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "integer";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::EMPTY:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "null";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::TIME:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "number";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::DATE:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "string";
                            break;
                        case E_EXCEL_LOGICAL_TYPE::DATE_TIME:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "string";
                            break;
                        default:
                            js["properties"][sheetname]["items"]["properties"][header]["type"] = "string";
                        }
                        target_types.push_back(celltype);
                    }
                    schemaCreated = true;
                }
                irow++;
                continue;
            }
            std::vector<std::string> headerRow = getHeaderForRow(headers, irow);
            jsoncons::json jrow;
            size_t icell = 1;
			size_t iheader = 0;
            for (auto& ccell : row.cells())
            {
                auto& cell = ccell.value();
                if (icell - 1 < headerRow.size())
                {
                    std::string headerLabel = headerRow[icell - 1];
                    std::string header = to_js_name(headerLabel);
                    auto target_value_type = E_EXCEL_LOGICAL_TYPE::EMPTY;
                    auto source_value_type = get_logical_type(doc, ccell);
                    if (icell >= 1 && icell <= target_types.size())
                        target_value_type = target_types[icell-1];
                    
                    if (target_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                    {
                        jrow[header] = cell.getString();
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::STRING)
                    {
                        jrow[header] = cell.getString();
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::BOOLEAN)
                    {
                        if (source_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                        {
                            jrow[header] = jsoncons::json::null();
                        }
                        else if (source_value_type == E_EXCEL_LOGICAL_TYPE::BOOLEAN) {
                            try {
                                jrow[header] = cell.get<bool>();
                            }
                            catch (std::exception& e)
                            {
                                cerr << e.what() << std::endl;
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                        else {
                            try {
                                XLCellValue xcv = cell;
                                jrow[header] = std::lround(xcv.getDouble())>0;
                            }
                            catch (std::exception& e)
                            {
                                cerr << e.what() << std::endl;
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::INTEGER)
                    {
                        if (source_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                        {
                            jrow[header] = jsoncons::json::null();
                        }
                        else if (source_value_type == E_EXCEL_LOGICAL_TYPE::INTEGER) {
                            try {
                                jrow[header] = cell.get<int>();
                            }
                            catch (std::exception& e)
                            {
                                cerr << e.what() << std::endl;
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                        else {
                            try {
                                XLCellValue xcv = cell;
                                jrow[header] = std::lround(xcv.getDouble());
                            }
                            catch (std::exception& e)
                            {
                                cerr << e.what() << std::endl;
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                        
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::DOUBLE)
                    { 
                        //auto dc=mynf.formatCode();
                        if (source_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                        {
                            jrow[header] = jsoncons::json::null();
                        }
                        else
                        {
                            try {
                                XLCellValue xcv = cell;
                                jrow[header] = xcv.getDouble();
                            }
                            catch (...)
                            {
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::TIME)
                    {
                        //auto dc=mynf.formatCode();
                        if (source_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                        {
                            jrow[header] = jsoncons::json::null();
                        }
                        else
                        {
                            try {
                                XLCellValue xcv = cell;
                                jrow[header] = xcv.getDouble();
                            }
                            catch (...)
                            {
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                    }
                    else if (target_value_type == E_EXCEL_LOGICAL_TYPE::DATE || target_value_type == E_EXCEL_LOGICAL_TYPE::DATE_TIME)
                    {
                        //auto dc=mynf.formatCode();
                        if (source_value_type == E_EXCEL_LOGICAL_TYPE::EMPTY)
                        {
                            jrow[header] = jsoncons::json::null();
                        }
                        else
                        {
                            try {
                                XLCellValue xcv = cell;
                                auto dth= xcv.get<XLDateTime>();
                                auto tm=dth.tm();
                                std::time_t t = std::mktime(&tm);
                                char buf[sizeof "2011-10-08T07:07:09Z"];
                                if (target_value_type == E_EXCEL_LOGICAL_TYPE::DATE_TIME)
                                {
                                    strftime(buf, sizeof buf, "%FT%TZ", std::gmtime(&t));
                                }
                                else
                                {
                                    strftime(buf, sizeof buf, "%F", std::localtime(&t));
								}
                                jrow[header] = buf;

                            }
                            catch (...)
                            {
                                jrow[header] = jsoncons::json::null();
                            }
                        }
                    }
                    else
                    {
                        jrow[header] = cell.getString();
                    }

                }
                //std::clog << cell.to_string() << std::endl;
                icell++;
            }
            
            for (size_t i = icell; i <= headerRow.size(); i++)
            {
				auto headerLabel= headerRow[i-1];
				auto target_value_type = E_EXCEL_LOGICAL_TYPE::EMPTY;
                if (i >= 1 && i <= target_types.size())
                    target_value_type = target_types[icell - 1];
                std::string header = to_js_name(headerLabel);
                if (target_value_type == E_EXCEL_LOGICAL_TYPE::STRING)
                {
                    jrow[header] = "";
                }
                else
                {
                    jrow[header] = jsoncons::json::null();
                }
			}

            std::stringstream sst;
            jrow.dump(sst);
            values.push_back(sst.str());


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
            cache[sheetname] = values;
        }
    }
    {
        schema= js.to_string();
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
   // std::cout << "Time taken: " << duration.count() * 1.0 / 1000.0 / 1000.0 << " microseconds" << std::endl;
   // std::clog << "Processing complete" << std::endl;
    return true;

}

ExcelImporter::ExcelImporter()
{
}