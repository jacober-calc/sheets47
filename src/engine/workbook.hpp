// Original Sheets47 code, MIT license.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sheets47 {
inline constexpr unsigned MaxRows = 256, MaxCols = 26, MaxCells = 256, MaxInput = 143;
inline constexpr const char* Version = "0.1.0";
inline constexpr const char* DataDirectory = "/SHEETS47";

struct Cell {
    uint16_t row = 0;
    uint8_t col = 0;
    friend bool operator==(Cell, Cell) = default;
};
struct Area {
    Cell first{}, last{};
    static Area between(Cell, Cell);
};
enum class Format : uint8_t { General, Fixed2, Scientific };
struct Record {
    Cell cell;
    Format format = Format::General;
    std::string input;
};
std::string address(Cell);
bool parse_address(const std::string&, Cell&);

// All mutations build and calculate a candidate before swapping it into place.
// The bounded source records are persistence/transaction metadata. Spreader
// alone owns calculated values, parsing, references, and spreadsheet semantics.
class Workbook {
public:
    Workbook();
    ~Workbook();
    Workbook(Workbook&&) noexcept;
    Workbook& operator=(Workbook&&) noexcept;
    Workbook(const Workbook&) = delete;
    Workbook& operator=(const Workbook&) = delete;
    bool set(Cell, const std::string&);
    bool clear(Area);
    bool copy(Area, Cell);
    bool paste(const std::vector<Record>&, Area, Cell);
    bool fill_down(Area);
    bool insert_row(unsigned);
    bool erase_row(unsigned);
    bool set_format(Area, Format);
    bool replace(const std::vector<Record>&);
    bool demo();
    std::string input(Cell) const;
    std::string display(Cell) const;
    bool numeric(Cell) const;
    Format format(Cell) const;
    const std::vector<Record>& records() const;
    const char* error() const { return error_; }
    uint32_t generation() const { return generation_; }
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    char error_[96]{};
    uint32_t generation_ = 0;
    bool commit(std::vector<Record>);
    bool structural(Area, Cell, int);
    bool fail(const char*);
};
}
