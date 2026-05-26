#ifndef EXCEL_HELPER_H
#define EXCEL_HELPER_H
#include <chrono>
#include <OpenXLSX.hpp>
using namespace std;
using namespace OpenXLSX;

enum E_EXCEL_LOGICAL_TYPE
{
    EMPTY,
    BOOLEAN,
    DOUBLE,
    INTEGER,
    DATE,
    TIME,
    DATE_TIME,
    STRING,
    ERROR
};

E_EXCEL_LOGICAL_TYPE get_logical_type(XLDocument& doc, OpenXLSX::XLCell& cell);
void add_cell_type(std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols, size_t irow, size_t icol, E_EXCEL_LOGICAL_TYPE celltype);

E_EXCEL_LOGICAL_TYPE get_cell_type(std::vector<std::vector<E_EXCEL_LOGICAL_TYPE>>& typecols, size_t irow_header, size_t icol);


std::string to_js_name(const std::string& label);
std::string format_utc(const std::chrono::zoned_seconds& zs);
void print_local(const std::chrono::zoned_seconds& zs);
#endif