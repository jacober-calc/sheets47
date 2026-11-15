// Original Sheets47 code, MIT license. Host paths stay under the supplied root.
#include "files.hpp"
#include <fstream>

namespace sheets47 {
bool HostFiles::prepare(){std::error_code ec;std::filesystem::create_directories(root_,ec);return !ec;}
ReadResult HostFiles::read(const std::string& name,std::vector<uint8_t>& data,size_t max){
    if(name.find('/')!=std::string::npos||name.find("..")!=std::string::npos)return ReadResult::Error;
    std::error_code ec;auto path=root_/name;
    if(!std::filesystem::exists(path,ec))return ec?ReadResult::Error:ReadResult::Missing;
    auto size=std::filesystem::file_size(path,ec);if(ec||size>max)return ReadResult::Error;
    std::ifstream file(path,std::ios::binary);if(!file)return ReadResult::Error;
    data.resize(size);file.read(reinterpret_cast<char*>(data.data()),std::streamsize(size));
    return file?ReadResult::Ok:ReadResult::Error;
}
bool HostFiles::write(const std::string& name,const std::vector<uint8_t>& data,bool exclusive){
    if(name.find('/')!=std::string::npos||name.find("..")!=std::string::npos)return false;
    auto path=root_/name;std::error_code ec;
    if(exclusive&&std::filesystem::exists(path,ec))return false;
    if(ec)return false;
    std::ofstream file(path,std::ios::binary|std::ios::trunc);if(!file)return false;
    file.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));file.close();return bool(file);
}
}
