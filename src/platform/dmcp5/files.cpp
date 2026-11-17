// Original Sheets47 code, MIT license. No deletion or persistent write-enable.
#include "files.hpp"
#include <cstdio>
#include "sdk.hpp"

namespace sheets47::dmcp5 {
namespace {
std::string path(const std::string& name) { return std::string(DataDirectory)+"/"+name; }
class WriteAccess {
    int previous_;
public:
    WriteAccess() : previous_(sys_is_disk_write_enable()) { sys_disk_write_enable(1); }
    ~WriteAccess() { sys_disk_write_enable(previous_); }
    bool enabled() const { return sys_is_disk_write_enable()!=0; }
};
}
bool DeviceFiles::fail(const char* operation,int result) {
    std::snprintf(error_,sizeof error_,"%s (code %d); worksheet retained",operation,result);
    return false;
}
bool DeviceFiles::prepare() {
    error_[0]=0;
    if(!sys_disk_ok())return fail("Disk not ready",FR_NOT_READY);
    WriteAccess access;
    if(!access.enabled())return fail("Disk write access refused",FR_WRITE_PROTECTED);
    auto result=check_create_dir(DataDirectory);
    return result==0 || fail("Cannot create /SHEETS47",result);
}
ReadResult DeviceFiles::read(const std::string& name,std::vector<uint8_t>& data,size_t max) {
    error_[0]=0;
    auto filename=path(name);
    FIL file;auto result=f_open(&file,filename.c_str(),FA_READ);
    if(result==FR_NO_FILE||result==FR_NO_PATH)return ReadResult::Missing;
    if(result!=FR_OK){fail("File open failed",result);return ReadResult::Error;}
    try{
        auto length=f_size(&file);
        if(length>max){f_close(&file);fail("File exceeds size limit",FR_INVALID_PARAMETER);return ReadResult::Error;}
        data.resize(length);UINT count=0;
        auto read_result=f_read(&file,data.data(),UINT(data.size()),&count);
        auto close_result=f_close(&file);
        if(read_result!=FR_OK||close_result!=FR_OK||count!=data.size()){
            fail("File read/close failed",read_result!=FR_OK?read_result:close_result!=FR_OK?close_result:FR_DISK_ERR);
            return ReadResult::Error;
        }
        return ReadResult::Ok;
    }catch(...){f_close(&file);throw;}
}
bool DeviceFiles::write(const std::string& name,const std::vector<uint8_t>& data,bool create_new) {
    error_[0]=0;
    if(!sys_disk_ok())return fail("Disk not ready",FR_NOT_READY);
    auto filename=path(name);
    WriteAccess access;
    if(!access.enabled())return fail("Disk write access refused",FR_WRITE_PROTECTED);
    FIL file;auto opened=f_open(&file,filename.c_str(),FA_WRITE|(create_new?FA_CREATE_NEW:FA_CREATE_ALWAYS));
    if(opened!=FR_OK)return fail("File write open failed",opened);
    UINT count=0;auto result=f_write(&file,data.data(),UINT(data.size()),&count);
    auto closed=f_close(&file); // Flush/close before restoring firmware protection.
    if(result!=FR_OK||count!=data.size()||closed!=FR_OK)
        return fail("File write/close failed",result!=FR_OK?result:closed!=FR_OK?closed:FR_DISK_ERR);
    return true;
}
}
