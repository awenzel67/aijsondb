#include <chrono>
#include <codecvt>
#include <locale>
#include <set>
#include "excelhelper.h"
#include "aijsondbimportexcel.h"
#include <OpenXLSX.hpp>

using namespace std;
using namespace OpenXLSX;

E_EXCEL_LOGICAL_TYPE get_logical_type(XLDocument& doc, OpenXLSX::XLCell& cell)
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

        bool isTime = false;
        bool isDate = false;
        auto format = cell.cellFormat();
        auto mystyles = doc.styles();
        auto ff = mystyles.cellFormats()[format];
        auto nf = ff.numberFormatId();
        auto mynf = mystyles.numberFormats();
        for (size_t index = 0; index < mynf.count(); ++index) {
            auto sfm = mynf.numberFormatByIndex(index);
            if (sfm.numberFormatId() == nf)
            {

                //auto sfm = mynf.numberFormatById(nf);
                auto code = sfm.formatCode();
                isTime = (code.find("hh") != std::string::npos) || (code.find("HH") != std::string::npos) || (code.find("h") != std::string::npos) || (code.find("H") != std::string::npos);
                isDate = (code.find("dd") != std::string::npos) || (code.find("DD") != std::string::npos) || (code.find("yy") != std::string::npos) || (code.find("yy") != std::string::npos);
            }
        }


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
    for (size_t icolr = 0; icolr < typecols.size(); icolr++)
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

    E_EXCEL_LOGICAL_TYPE valmax = E_EXCEL_LOGICAL_TYPE::STRING;
    size_t count_max = 0;

    bool hasDateTomeAndDate =
        typecounts.find(E_EXCEL_LOGICAL_TYPE::DATE_TIME) != typecounts.end()
        && typecounts.find(E_EXCEL_LOGICAL_TYPE::DATE) != typecounts.end()
        ;

    if (hasDateTomeAndDate)
    {
        typecounts[E_EXCEL_LOGICAL_TYPE::DATE_TIME] = typecounts[E_EXCEL_LOGICAL_TYPE::DATE_TIME] + typecounts[E_EXCEL_LOGICAL_TYPE::DATE];
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
        if (typecount.first != E_EXCEL_LOGICAL_TYPE::EMPTY && typecount.second > count_max)
        {
            valmax = typecount.first;
            count_max = typecount.second;
        }
    }

    return valmax;
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
            if (isalpha(c) || c == '_')
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

std::string format_utc(const std::chrono::zoned_seconds& zs) {
    auto tp = zs.get_sys_time();
    auto dp = std::chrono::floor<std::chrono::days>(tp);
    auto ymd = std::chrono::year_month_day{ std::chrono::sys_days{dp} };
    auto time = std::chrono::hh_mm_ss{ tp - dp };
    std::ostringstream oss;
    oss << static_cast<int>(ymd.year()) << "-"
        << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
        << std::setw(2) << static_cast<unsigned>(ymd.day()) << "T"
        << std::setw(2) << time.hours().count() << ":"
        << std::setw(2) << time.minutes().count() << ":"
        << std::setw(2) << time.seconds().count() << "Z";
    return oss.str();
}

void print_local(const std::chrono::zoned_seconds& zs) {
    auto lt = zs.get_local_time();
    auto tz = zs.get_time_zone();
    auto dp = std::chrono::floor<std::chrono::days>(lt);
    auto ymd = std::chrono::year_month_day{ std::chrono::local_days{dp} };
    auto time = std::chrono::hh_mm_ss{ lt - dp };
    std::cout << tz->name() << " local: "
        << static_cast<int>(ymd.year()) << "-"
        << std::setfill('0') << std::setw(2) << static_cast<unsigned>(ymd.month()) << "-"
        << std::setw(2) << static_cast<unsigned>(ymd.day()) << " "
        << std::setw(2) << time.hours().count() << ":"
        << std::setw(2) << time.minutes().count() << ":"
        << std::setw(2) << time.seconds().count() << "\n";
}