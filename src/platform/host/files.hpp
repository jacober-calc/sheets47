// Original Sheets47 code, MIT license.
#pragma once
#include "storage/storage.hpp"
#include <filesystem>

namespace sheets47 {
class HostFiles final:public FilePort {
public:
    explicit HostFiles(std::filesystem::path root):root_(std::move(root)){}
    bool prepare() override;
    ReadResult read(const std::string&,std::vector<uint8_t>&,size_t) override;
    bool write(const std::string&,const std::vector<uint8_t>&,bool) override;
private:
    std::filesystem::path root_;
};
}
