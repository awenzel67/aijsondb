#include <OpenXLSX.hpp>
#include "aijsondbimportexcel.h"
#include "excelhelper.h"
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
#include <format>
#include <filesystem>
#include <time.h>
#include "excelhelper.h"


typedef std::tuple<int, std::vector<std::string>> HeaderInfo;

size_t countDistinct(const std::vector<std::string>& vec) {
    //std::set<std::string> distinctSet(vec.begin(), vec.end());
    return vec.size();
}

bool getHeaders2Plus(XLDocument& doc, XLWorksheet& ws, std::map<int,HeaderInfo>& headers, std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols)
{
    //std::map<int, std::vector<std::string>> headers;
    std::clog << "Processing spread sheet" << std::endl;
    bool isHeaderLast = false;
    std::vector<std::string> headerRowMax;
    size_t irow = 0;
    int irowHeader = -1;
	int icolHeaderStart = -1;
    typecols.clear();
    //std::vector<std::vector<int>> typecols;
    for (auto& row : ws.rows())
    {
        bool isHeader = false;
        std::vector<std::string> headerRow;
        int icollast = -1;
        int icolFirstEmpty = -1;
		int icolFirstNonEmpty = -1;
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
                if (icolFirstNonEmpty == -1) {
                    icolFirstNonEmpty = icell;
                }
            }
            else if (cell.type() == OpenXLSX::XLValueType::Empty)
            {
                if (icolFirstEmpty == -1 && icolFirstNonEmpty != -1) {
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

        isHeader = icollast >= 0 && (icolFirstEmpty == -1 || icolFirstEmpty > icollast) && !hasNonHeadrCells && icolFirstNonEmpty != -1;

        if (isHeader)
        {
            if ( countDistinct(headerRowMax) < countDistinct(headerRow))
            {
                headerRowMax = headerRow;
                irowHeader = irow;
                icolHeaderStart=icolFirstNonEmpty;
            }
        }
        irow++;
    }
    if (irowHeader != -1)
    {
		HeaderInfo headerInfo(icolHeaderStart,headerRowMax);
        headers[irowHeader] = headerInfo;
    }
    std::clog << "Processing complete" << std::endl;
    return true;
}

bool isHeadersPlus(std::map<int,HeaderInfo>& headers, int irow)
{
    if (headers.find(irow) != headers.end())
    {
        return true;
    }
    return false;
}

bool isBeforePlus(std::map<int,HeaderInfo>& headers, int irow)
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

HeaderInfo  getHeaderForRowPlus(std::map<int, HeaderInfo>& headers, int irow)
{
    HeaderInfo empty;
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


bool uncollapseCells(XLDocument& doc, XLWorksheet& ws)
{
    auto& mergedRanges = ws.merges();
    size_t count = mergedRanges.count();

    std::cout << "Gefundene Merge-Bereiche:\n";
    for (size_t i = 0; i < count; i++) {
        // range ist vom Typ XLCellRange
        auto range = mergedRanges[i];
        std::cout << "Merge-Bereich " << i << ": " << range << "\n";
        XLCellRange cellRange = ws.range(range);
        XLCell topLeftCell = ws.cell(cellRange.topLeft());
        for (XLCellIterator cellIt = cellRange.begin(); cellIt != cellRange.end(); ++cellIt) {
            if (!cellIt.cellExists()) continue; // prevent cell creation by access for non-existing cells

            if (cellIt.address() == topLeftCell.cellReference().address()) {
                // This is the top-left cell of the merged range, keep its value
                continue;
            }
            ws.cell(cellIt.address()).value() = topLeftCell.value(); // Copy value from the top-left cell to the current cell
        }
        /*
        auto topLeft = range.topLeft();
        auto bottomRight = range.bottomRight();

        std::cout << "Von "
            << topLeft.address() << " bis "
            << bottomRight.address();

        // Wert der linken oberen Zelle (enthält den Inhalt)
        auto value = wks.cell(topLeft).value();
        std::cout << " | Wert: " << value.get<std::string>() << "\n";
        */
    }
    mergedRanges.deleteAll();
    return true;
}

bool uncollapseCellsInDocument(XLDocument& doc)
{
    auto ws = doc.workbook().sheetCount();
    for (size_t iws = 0; iws < ws; iws++)
    {
        auto wks = doc.workbook().worksheet(iws + 1);
        uncollapseCells(doc, wks);
    }
    return true;
}

std::string make_individal_header(std::map<string,size_t>& headers,const std::string& header)
{
	auto it = headers.find(header);
	size_t index = 1;
    if (it != headers.end())
    {
        headers[header] = index;
        return header + "_" + std::to_string(index);
        index = it->second + 1;
    }
    else
    {
		headers[header] = index;
		return header;
    }
}

bool ExcelImporterPlus::import(const std::string& filepath, std::map<std::string, std::vector<std::string>>& cache, std::string& schema, std::string& error)
{
    cache.clear();
    schema.clear();
    XLDocument doc;
    //doc.create("./Demo01.xlsx", XLForceOverwrite);
    std::filesystem::path apa(filepath);
    std::string filename(apa.filename().string());
    auto start = std::chrono::high_resolution_clock::now();

    try {
        doc.open(filepath);
		uncollapseCellsInDocument(doc);
    }
    catch (std::exception& ex)
    {
        error = ex.what();
        return false;
    }

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
        auto ws = doc.workbook().worksheet(iws + 1);
        std::string sheetlabel = ws.name();
        std::string sheetname = to_js_name(sheetlabel);
        auto jsArrayRow = jsoncons::json::object();
        std::map<int, HeaderInfo> headers;
        std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>> typecols;
        getHeaders2Plus(doc, ws, headers, typecols);
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
            if (isBeforePlus(headers, irow))
            {
                irow++;
                continue;
            }

            bool isHeader = isHeadersPlus(headers, irow);
            if (isHeader)
            {
                if (!schemaCreated) {
                    HeaderInfo headerRow = getHeaderForRowPlus(headers, irow);
					std::map<std::string,size_t> headerMap;
                    for (size_t ih = 0; ih < std::get<1>(headerRow).size(); ih++)
                    {
						int i = ih + std::get<0>(headerRow);
                        std::string headerLabel = std::get<1>(headerRow)[ih];
                        std::string header = to_js_name(std::get<1>(headerRow)[ih]);
                        header = make_individal_header(headerMap, header);
						
                        js["properties"][sheetname]["items"]["properties"][header] = jsoncons::json::object();
                        js["properties"][sheetname]["items"]["properties"][header]["description"] = headerLabel;
                        auto celltype = get_cell_type(typecols, irow, i);
                        switch (celltype)
                        {
                        case E_EXCEL_LOGICAL_TYPE::STRING:
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
            HeaderInfo headerRow = getHeaderForRowPlus(headers, irow);
            jsoncons::json jrow;
            size_t icell = 1;
            size_t iheader = 0;
            std::map<std::string, size_t> headerMap;
            for (auto& ccell : row.cells())
            {
                auto& cell = ccell.value();
				int icellStart = std::get<0>(headerRow);
				int icellEnd = std::get<0>(headerRow) + std::get<1>(headerRow).size();
                if (icell - 1 < icellEnd && icell - 1 >= icellStart)
                {
                    int iColHead = icell - 1 - icellStart;
                    std::string headerLabel = std::get<1>(headerRow)[iColHead];
                    std::string header = to_js_name(headerLabel);
                    header = make_individal_header(headerMap, header);
                    auto target_value_type = E_EXCEL_LOGICAL_TYPE::EMPTY;
                    auto source_value_type = get_logical_type(doc, ccell);
                    if (iColHead>=0 && iColHead < target_types.size())
                        target_value_type = target_types[iColHead];

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
                                jrow[header] = std::lround(xcv.getDouble()) > 0;
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
                                auto dth = xcv.get<XLDateTime>();
                                auto tm = dth.tm();
                                std::time_t t = std::mktime(&tm);
                                std::string sdatetime;
                                if (target_value_type == E_EXCEL_LOGICAL_TYPE::DATE_TIME)
                                {
                                    //      if (t >= 0)
                                    //      {
                                    //          char buf[sizeof "2011-10-08T07:07:09Z"];
                                    //          strftime(buf, sizeof buf, "%FT%TZ", std::gmtime(&t));
                                    //			sdatetime = buf;
                                    //      }
                                    //      else
                                    {
                                        const auto* local_tz = std::chrono::current_zone();
                                        auto ymd = std::chrono::year_month_day{
                                            std::chrono::year{tm.tm_year + 1900},
                                            std::chrono::month{static_cast<unsigned>(tm.tm_mon + 1)},
                                            std::chrono::day{static_cast<unsigned>(tm.tm_mday)}
                                        };
                                        auto local_time = std::chrono::local_days{ ymd } +
                                            std::chrono::hours{ tm.tm_hour } +
                                            std::chrono::minutes{ tm.tm_min } +
                                            std::chrono::seconds{ tm.tm_sec };

                                        // Convert to sys_time (UTC)
                                        auto zt = std::chrono::zoned_time{ local_tz, local_time };
                                        sdatetime = format_utc(zt);
                                    }
                                }
                                else
                                {
                                    char buf[sizeof "2011-10-08T07:07:09Z"];
                                    strftime(buf, sizeof buf, "%F", &tm);
                                    sdatetime = buf;
                                }
                                jrow[header] = sdatetime;

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

			// Fill in missing cells with null or empty string based on target type

            for (size_t i = icell; i <= std::get<0>(headerRow) + std::get<1>(headerRow).size(); i++)
            {
                if (i - 1 < std::get<0>(headerRow)) 
                {
                    continue; // Skip cells that are outside the header range
				}
				int iColHead = i - 1 - std::get<0>(headerRow);
                auto headerLabel = std::get<1>(headerRow)[iColHead];
                auto target_value_type = E_EXCEL_LOGICAL_TYPE::EMPTY;
                if (iColHead >= 0 && iColHead < target_types.size())
                    target_value_type = target_types[iColHead];
                std::string header = to_js_name(headerLabel);
                header = make_individal_header(headerMap, header);
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
        schema = js.to_string();
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    // std::cout << "Time taken: " << duration.count() * 1.0 / 1000.0 / 1000.0 << " microseconds" << std::endl;
    // std::clog << "Processing complete" << std::endl;
    return true;

}

ExcelImporterPlus::ExcelImporterPlus()
{
}



bool testUncollapseCells(std::string& excel_in,std::string&  excel_out)
{
    XLDocument doc;
    try {
        doc.open(excel_in);
		uncollapseCellsInDocument(doc);
		doc.saveAs(excel_out);
    }
    catch (std::exception& ex)
    {

        return false;
    }
	return true;
}

