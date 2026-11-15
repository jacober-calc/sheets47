// Original Sheets47 code, MIT license. Pinned Spreader patches are documented.
#include "workbook.hpp"
#include <spreader/sheet.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace sheets47 {
namespace {
using Spreader::Sheet;
using Spreader::String;
Spreader::Point point(Cell c) { return {c.col, c.row}; }
bool valid(Cell c) { return c.col < MaxCols && c.row < MaxRows; }
bool contains(Area a, Cell c) {
    return c.row >= a.first.row && c.row <= a.last.row && c.col >= a.first.col && c.col <= a.last.col;
}
std::string ascii(const String& value) {
    std::string out;
    for (char32_t c : String::utf32_view(value)) out += c >= 32 && c <= 126 ? char(c) : '?';
    return out;
}
bool allowed_formula(const std::string& input) {
    static const char* const supported[] = {"SUM", "AVERAGE", "MIN", "MAX", "COUNT", "IF", "ROUND", "ROUNDUP", "ROUNDDOWN"};
    unsigned nesting = 0;
    bool reference = false;
    for (size_t i = 1; i < input.size();) {
        unsigned char c = input[i];
        if (input.compare(i,5,"#REF!")==0) { i+=5; reference=true; }
        else if (c == ':') {
            if (!reference) return false;
            size_t next=i+1;
            while(next<input.size() && input[next]==' ')++next;
            if(next<input.size() && input[next]=='$')++next;
            if(next>=input.size() || !std::isalpha(static_cast<unsigned char>(input[next])))return false;
            ++i; reference=false;
        } else if (c == '"') {
            reference=false;
            ++i;
            while (i < input.size()) {
                if (input[i++] == '"') {
                    if (i < input.size() && input[i] == '"') ++i;
                    else break;
                }
            }
        } else if (std::isdigit(c) || c == '.') {
            reference=false;
            char* end = nullptr;
            std::strtod(input.c_str() + i, &end);
            if (end == input.c_str() + i) ++i;
            else i = size_t(end - input.c_str());
        } else if (c == '$' || std::isalpha(c)) {
            std::string token;
            if (c == '$') ++i;
            while (i < input.size() && std::isalpha(static_cast<unsigned char>(input[i])))
                token += char(std::toupper(static_cast<unsigned char>(input[i++])));
            size_t next = i;
            while (next < input.size() && input[next] == ' ') ++next;
            if (next < input.size() && input[next] == '(') {
                bool found = false;
                for (auto name : supported) if (token == name) found = true;
                if (!found) return false;
            } else if (token == "TRUE" || token == "FALSE") {
                // Literal Boolean operands.
            } else {
                if (i < input.size() && input[i] == '$') ++i;
                size_t start = i;
                unsigned row = 0;
                while (i < input.size() && std::isdigit(static_cast<unsigned char>(input[i]))) {
                    row = row * 10 + unsigned(input[i++] - '0');
                    if (row > MaxRows) return false;
                }
                if (token.size() != 1 || token[0] < 'A' || token[0] > 'Z' || i == start || row == 0) return false;
                reference=true;
            }
        } else {
            if(c!=' ')reference=false;
            if (c == '(' && ++nesting > 12) return false;
            if (c == ')' && nesting) --nesting;
            if (std::strchr(" +-*/^&%=<>:,()", c) == nullptr) return false;
            ++i;
        }
    }
    return true;
}
void validate(const Record& r) {
    if (!valid(r.cell) || unsigned(r.format) > unsigned(Format::Scientific) || r.input.size() > MaxInput)
        throw std::runtime_error("Cell, format or input exceeds limits");
    for (unsigned char c : r.input) if (c < 32 || c > 126) throw std::runtime_error("ASCII text only in version 0.1.0");
    if (!r.input.empty() && r.input.front() == '=' && !allowed_formula(r.input))
        throw std::runtime_error("Use A1:Z256 and supported functions; nesting max 12");
}
void set_engine(Sheet& sheet, const Record& r) {
    if (r.input.empty()) return;
    if (r.input[0] == '=') sheet.setFormulaCell(point(r.cell), String(r.input.c_str() + 1));
    else if (r.input[0] == '\'') sheet.setValueCell(point(r.cell), String(r.input.c_str() + 1));
    else {
        char* end = nullptr;
        double v = std::strtod(r.input.c_str(), &end);
        if (end != r.input.c_str() && *end == 0 && std::isfinite(v)) sheet.setValueCell(point(r.cell), v);
        else sheet.setValueCell(point(r.cell), String(r.input.c_str()));
    }
}
std::string cell_source(const Sheet& sheet, Cell c) {
    if (auto formula = sheet.getFormulaInfo(point(c))) return "=" + ascii(formula->text);
    auto value = sheet.getValue(point(c));
    return applyVisitor([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Spreader::Scalar::Blank>) return {};
        else if constexpr (std::is_same_v<T, String>) return "'" + ascii(v);
        else if constexpr (std::is_same_v<T, Spreader::Number>) return ascii(v.toString());
        else if constexpr (std::is_same_v<T, bool>) return v ? "=TRUE" : "=FALSE";
        else return "'" + ascii(toString(v));
    }, value);
}
}

struct Workbook::Impl {
    Sheet sheet;
    std::vector<Record> sources;
    explicit Impl(std::vector<Record> data) : sources(std::move(data)) {
        if (sources.size() > MaxCells) throw std::runtime_error("Limit: 256 populated cells");
        std::sort(sources.begin(), sources.end(), [](const Record& a, const Record& b) {
            return unsigned(a.cell.row) * MaxCols + a.cell.col < unsigned(b.cell.row) * MaxCols + b.cell.col;
        });
        sheet.suspendRecalc();
        for (size_t i = 0; i < sources.size(); ++i) {
            const auto& r = sources[i];
            validate(r);
            if (i && sources[i-1].cell == r.cell) throw std::runtime_error("Duplicate cell in workbook");
            set_engine(sheet, r);
        }
        sheet.resumeRecalc();
        for(const auto& r:sources)if(!r.input.empty()&&r.input.front()=='='){
            auto formula=sheet.getFormulaInfo(point(r.cell));
            if(formula&&(formula->extent.width>1||formula->extent.height>1))
                throw std::runtime_error("Array/spill formulas are not available in 0.1.0");
        }
        if (sheet.nonNullCellCount()>MaxCells || sheet.size().width>MaxCols || sheet.size().height>MaxRows)
            throw std::runtime_error("Calculated output exceeds worksheet limits");
    }
};

Area Area::between(Cell a, Cell b) {
    return {{std::min(a.row,b.row), std::min(a.col,b.col)}, {std::max(a.row,b.row),std::max(a.col,b.col)}};
}
std::string address(Cell c) { return std::string(1, char('A' + c.col)) + std::to_string(c.row + 1); }
bool parse_address(const std::string& value, Cell& cell) {
    if (value.size() < 2 || value.size() > 4) return false;
    char col = char(std::toupper(static_cast<unsigned char>(value[0])));
    if (col < 'A' || col > 'Z') return false;
    unsigned row = 0;
    for (size_t i=1; i<value.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return false;
        row = row*10 + unsigned(value[i]-'0');
    }
    if (!row || row > MaxRows) return false;
    cell = {uint16_t(row-1),uint8_t(col-'A')};
    return true;
}
Workbook::Workbook() : impl_(std::make_unique<Impl>(std::vector<Record>{})) {}
Workbook::~Workbook() = default;
Workbook::Workbook(Workbook&&) noexcept = default;
Workbook& Workbook::operator=(Workbook&&) noexcept = default;
bool Workbook::fail(const char* message) { std::snprintf(error_, sizeof(error_), "%s", message); return false; }
bool Workbook::commit(std::vector<Record> data) {
    auto candidate = std::make_unique<Impl>(std::move(data));
    impl_.swap(candidate);
    error_[0] = 0;
    ++generation_;
    return true;
}
bool Workbook::replace(const std::vector<Record>& data) {
    try { return commit(data); }
    catch (const std::bad_alloc&) { return fail("Memory full; worksheet unchanged"); }
    catch (const std::exception& e) { return fail(e.what()); }
}
bool Workbook::set(Cell cell, const std::string& input) {
    try {
        if (!valid(cell)) return fail("Cell outside A1:Z256");
        if (this->input(cell) == input) { error_[0]=0; return true; }
        auto data = impl_->sources;
        auto it = std::find_if(data.begin(), data.end(), [cell](const Record& r) { return r.cell==cell; });
        if (it != data.end()) {
            if (input.empty()) data.erase(it);
            else it->input = input;
        } else if (!input.empty()) data.push_back({cell, Format::General, input});
        return commit(std::move(data));
    } catch (const std::bad_alloc&) { return fail("Memory full; worksheet unchanged"); }
      catch (const std::exception& e) { return fail(e.what()); }
}
bool Workbook::clear(Area area) {
    try {
        auto data = impl_->sources;
        data.erase(std::remove_if(data.begin(), data.end(), [area](const Record& r) { return contains(area,r.cell); }), data.end());
        return commit(std::move(data));
    } catch (const std::bad_alloc&) { return fail("Memory full; worksheet unchanged"); }
      catch (const std::exception& e) { return fail(e.what()); }
}
bool Workbook::set_format(Area area, Format f) {
    try {
        auto data = impl_->sources;
        for (auto& r : data) if (contains(area,r.cell)) r.format = f;
        return commit(std::move(data));
    } catch (const std::bad_alloc&) { return fail("Memory full; worksheet unchanged"); }
      catch (const std::exception& e) { return fail(e.what()); }
}
bool Workbook::structural(Area area, Cell destination, int mode) {
    try {
        if (!valid(area.first) || !valid(area.last) || !valid(destination)) return fail("Cell outside A1:Z256");
        auto candidate = std::make_unique<Impl>(impl_->sources);
        unsigned width = area.last.col-area.first.col+1, height = area.last.row-area.first.row+1;
        if (mode == 0) {
            if (destination.col+width > MaxCols || destination.row+height > MaxRows) return fail("Paste exceeds A1:Z256");
            candidate->sheet.copyCells({point(area.first), {width,height}}, point(destination));
        } else if (mode==3) {
            for(unsigned col=area.first.col;col<=area.last.col;++col)
                candidate->sheet.copyCell(point({area.first.row,uint8_t(col)}),{point({area.first.row,uint8_t(col)}),{1,height}});
        } else if (mode == 1) {
            for (const auto& r : impl_->sources) if (r.cell.row == MaxRows-1) return fail("Last row occupied; insert would lose data");
            candidate->sheet.insertRows(destination.row, 1);
        } else candidate->sheet.deleteRows(destination.row, 1);
        if (candidate->sheet.size().width > MaxCols || candidate->sheet.size().height > MaxRows || candidate->sheet.nonNullCellCount() > MaxCells)
            return fail("Edit exceeds worksheet limits");
        std::vector<Record> data;
        for (unsigned row=0; row<MaxRows; ++row) for (unsigned col=0; col<MaxCols; ++col) {
            Cell c{uint16_t(row),uint8_t(col)};
            auto source=cell_source(candidate->sheet,c);
            if (source.empty()) continue;
            Cell old=c;
            if(mode==3 && contains(area,c))old.row=area.first.row;
            else if (mode==1 && row>=destination.row) {
                if (row==destination.row) continue;
                --old.row;
            } else if (mode==2 && row>=destination.row) ++old.row;
            else if (mode==0 && row>=destination.row && row<destination.row+height && col>=destination.col && col<destination.col+width)
                old={uint16_t(area.first.row+row-destination.row),uint8_t(area.first.col+col-destination.col)};
            Record record{c,format(old),std::move(source)};
            // A structural edit may deliberately create #REF!; its reconstructed
            // formula must remain editable and reloadable, so validation accepts it.
            data.push_back(std::move(record));
        }
        candidate->sources=std::move(data);
        for (const auto& r : candidate->sources) validate(r);
        impl_.swap(candidate);
        ++generation_; error_[0]=0; return true;
    } catch (const std::bad_alloc&) { return fail("Memory full; worksheet unchanged"); }
      catch (const std::exception& e) { return fail(e.what()); }
}
bool Workbook::copy(Area area, Cell destination) { return structural(area,destination,0); }
bool Workbook::fill_down(Area area) { return structural(area,area.first,3); }
bool Workbook::paste(const std::vector<Record>& captured, Area area, Cell destination) {
    try {
        unsigned width=area.last.col-area.first.col+1, height=area.last.row-area.first.row+1;
        if(!valid(destination)||destination.col+width>MaxCols||destination.row+height>MaxRows)return fail("Paste exceeds A1:Z256");
        Impl clipboard(captured);
        clipboard.sheet.copyCells({point(area.first),{width,height}},point(destination));
        auto data=impl_->sources;
        Area target{destination,{uint16_t(destination.row+height-1),uint8_t(destination.col+width-1)}};
        data.erase(std::remove_if(data.begin(),data.end(),[target](const Record& r){return contains(target,r.cell);}),data.end());
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            Cell cell{uint16_t(destination.row+y),uint8_t(destination.col+x)};
            auto source=cell_source(clipboard.sheet,cell);
            if(source.empty())continue;
            Format fmt=Format::General;
            Cell origin{uint16_t(area.first.row+y),uint8_t(area.first.col+x)};
            for(const auto& r:captured)if(r.cell==origin)fmt=r.format;
            data.push_back({cell,fmt,std::move(source)});
        }
        return commit(std::move(data));
    }catch(const std::bad_alloc&){return fail("Memory full; worksheet unchanged");}
     catch(const std::exception& e){return fail(e.what());}
}
bool Workbook::insert_row(unsigned row) { if(row>=MaxRows)return fail("Row outside limits"); return structural({}, {uint16_t(row),0},1); }
bool Workbook::erase_row(unsigned row) { if(row>=MaxRows)return fail("Row outside limits"); return structural({}, {uint16_t(row),0},2); }
std::string Workbook::input(Cell c) const { for (const auto& r:impl_->sources) if(r.cell==c)return r.input; return {}; }
Format Workbook::format(Cell c) const { for(const auto& r:impl_->sources)if(r.cell==c)return r.format; return Format::General; }
const std::vector<Record>& Workbook::records() const { return impl_->sources; }
bool Workbook::numeric(Cell c) const { auto v=impl_->sheet.getValue(point(c)); return get<Spreader::Number>(&v)!=nullptr; }
std::string Workbook::display(Cell c) const {
    try {
        auto value=impl_->sheet.getValue(point(c));
        return applyVisitor([this,c](const auto& v)->std::string {
            using T=std::decay_t<decltype(v)>;
            if constexpr(std::is_same_v<T,Spreader::Scalar::Blank>)return {};
            else if constexpr(std::is_same_v<T,Spreader::Number>) {
                char buf[48]; auto f=format(c);
                if(f==Format::Fixed2) { std::snprintf(buf,sizeof buf,"%.2f",v.value()); return buf; }
                if(f==Format::Scientific) { std::snprintf(buf,sizeof buf,"%.5e",v.value()); return buf; }
                return ascii(v.toString());
            } else if constexpr(std::is_same_v<T,String>)return ascii(v);
            else if constexpr(std::is_same_v<T,bool>)return v?"TRUE":"FALSE";
            else return ascii(toString(v));
        },value);
    } catch(const std::bad_alloc&) { return "#MEM!"; }
}
bool Workbook::demo() {
    return replace({{{0,0},Format::General,"Item"},{{0,1},Format::General,"Qty"},{{0,2},Format::General,"Price"},{{0,3},Format::General,"Total"},
        {{1,0},Format::General,"Bolts"},{{1,1},Format::General,"3"},{{1,2},Format::Fixed2,"2.5"},{{1,3},Format::Fixed2,"=B2*C2"},
        {{2,0},Format::General,"Washers"},{{2,1},Format::General,"4"},{{2,2},Format::Fixed2,"0.75"},{{2,3},Format::Fixed2,"=B3*C3"},
        {{3,0},Format::General,"Brackets"},{{3,1},Format::General,"2"},{{3,2},Format::Fixed2,"4"},{{3,3},Format::Fixed2,"=B4*C4"},
        {{5,0},Format::General,"TOTAL"},{{5,3},Format::Fixed2,"=SUM(D2:D4)"}});
}
}
