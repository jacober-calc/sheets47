// Original Sheets47 code, MIT license.
#pragma once
#include "storage/storage.hpp"

namespace sheets47::dmcp5 {
class DeviceFiles final : public FilePort {
public:
    bool prepare() override;
    ReadResult read(const std::string&, std::vector<uint8_t>&, size_t) override;
    bool write(const std::string&, const std::vector<uint8_t>&, bool) override;
    const char* error() const override { return error_[0] ? error_ : nullptr; }
private:
    char error_[96]{};
    bool fail(const char* operation, int result);
};
}
