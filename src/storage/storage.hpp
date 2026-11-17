// Original Sheets47 code, MIT license.
#pragma once
#include "engine/workbook.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace sheets47 {
struct ViewState {
    Cell active{};
    uint16_t top = 0;
    uint8_t left = 0;
};
enum class ReadResult { Ok, Missing, Error };
class FilePort {
public:
    virtual ~FilePort() = default;
    virtual bool prepare() = 0;
    virtual const char* error() const { return nullptr; }
    virtual ReadResult read(const std::string& name, std::vector<uint8_t>& data, size_t max_bytes) = 0;
    virtual bool write(const std::string& name, const std::vector<uint8_t>& data, bool create_new) = 0;
};
enum class LoadResult { Loaded, Empty, Failed };
class Storage {
public:
    explicit Storage(FilePort& files) : files_(files) {}
    bool save(const Workbook&, const ViewState&);
    LoadResult load(Workbook&, ViewState&);
    const char* error() const { return error_; }
    uint32_t stored_generation() const { return generation_; }
private:
    FilePort& files_;
    uint32_t generation_ = 0;
    char error_[96]{};
    bool fail(const char*);
};
uint32_t crc32(const uint8_t*, size_t);
}
