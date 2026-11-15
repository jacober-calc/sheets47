// Original Sheets47 code, MIT license. Append-only generations; no unlink/rename.
#include "storage.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace sheets47 {
namespace {
using Bytes=std::vector<uint8_t>;
constexpr size_t MaxFile=65536;
void u16(Bytes& out,uint16_t v){out.push_back(uint8_t(v));out.push_back(uint8_t(v>>8));}
void u32(Bytes& out,uint32_t v){u16(out,uint16_t(v));u16(out,uint16_t(v>>16));}
uint16_t get16(const Bytes& b,size_t p){if(p+2>b.size())throw std::runtime_error("Truncated workbook");return uint16_t(b[p]|uint16_t(b[p+1])<<8);}
uint32_t get32(const Bytes& b,size_t p){return get16(b,p)|uint32_t(get16(b,p+2))<<16;}
void checksum(Bytes& out){u32(out,crc32(out.data(),out.size()));}
bool checked(const Bytes& b,const char* magic,size_t length){return b.size()>=length && std::memcmp(b.data(),magic,4)==0 && get32(b,b.size()-4)==crc32(b.data(),b.size()-4);}
std::string file_name(uint32_t n){char name[16];std::snprintf(name,sizeof name,"B%07lu.S47",static_cast<unsigned long>(n));return name;}
Bytes encode(const Workbook& workbook,const ViewState& view,uint32_t n){
    Bytes b{'S','4','7','W'};u16(b,1);u16(b,0);u32(b,n);
    u16(b,uint16_t(workbook.records().size()));u16(b,view.active.row);b.push_back(view.active.col);
    u16(b,view.top);b.push_back(view.left);
    for(const auto& r:workbook.records()){
        u16(b,r.cell.row);b.push_back(r.cell.col);b.push_back(uint8_t(r.format));u16(b,uint16_t(r.input.size()));
        b.insert(b.end(),r.input.begin(),r.input.end());
    }
    checksum(b);return b;
}
std::vector<Record> decode(const Bytes& b,uint32_t expected,ViewState& view){
    if(!checked(b,"S47W",24)||get16(b,4)!=1||get16(b,6)!=0||get32(b,8)!=expected)throw std::runtime_error("Unsupported or corrupt workbook");
    unsigned count=get16(b,12);if(count>MaxCells)throw std::runtime_error("Too many cells in workbook");
    view.active={get16(b,14),b[16]};view.top=get16(b,17);view.left=b[19];
    if(view.active.row>=MaxRows||view.active.col>=MaxCols||view.top>=MaxRows||view.left>=MaxCols)throw std::runtime_error("Invalid saved viewport");
    std::vector<Record> records;records.reserve(count);size_t p=20;
    for(unsigned i=0;i<count;++i){
        if(p+6>b.size()-4)throw std::runtime_error("Truncated cell record");
        Record r;r.cell={get16(b,p),b[p+2]};r.format=Format(b[p+3]);unsigned len=get16(b,p+4);p+=6;
        if(len>MaxInput||p+len>b.size()-4)throw std::runtime_error("Invalid cell length");
        r.input.assign(reinterpret_cast<const char*>(b.data()+p),len);p+=len;records.push_back(std::move(r));
    }
    if(p!=b.size()-4)throw std::runtime_error("Unexpected workbook bytes");
    return records;
}
uint32_t head_generation(FilePort& files,const char* name,bool& found,bool& io_error){
    Bytes b;auto result=files.read(name,b,64);
    if(result==ReadResult::Missing)return 0;
    found=true;
    if(result==ReadResult::Error){io_error=true;return 0;}
    if(b.size()!=12||!checked(b,"S47H",12))return 0;
    auto n=get32(b,4);return n>0&&n<=9999999?n:0;
}
}
uint32_t crc32(const uint8_t* data,size_t size){
    uint32_t crc=~0u;
    for(size_t i=0;i<size;++i){crc^=data[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xEDB88320u & (0u-(crc&1u)));}
    return ~crc;
}
bool Storage::fail(const char* error){std::snprintf(error_,sizeof error_,"%s",error);return false;}
bool Storage::save(const Workbook& workbook,const ViewState& view){
    try{
        if(!files_.prepare())return fail("Storage unavailable; worksheet is still unsaved");
        bool found=false,io_error=false;
        auto a=head_generation(files_,"HEAD0.S47",found,io_error),b=head_generation(files_,"HEAD1.S47",found,io_error);
        if(io_error)return fail("Cannot read save headers; save aborted");
        auto valid_generation=[this,&io_error](uint32_t version){
            if(!version)return false;
            Bytes bytes;auto read=files_.read(file_name(version),bytes,MaxFile);
            if(read==ReadResult::Error){io_error=true;return false;}
            if(read!=ReadResult::Ok)return false;
            try{ViewState candidate;decode(bytes,version,candidate);return true;}
            catch(const std::runtime_error&){return false;}
        };
        bool valid_a=valid_generation(a),valid_b=valid_generation(b);
        if(io_error)return fail("Cannot validate prior save; save aborted");
        uint32_t n=std::max({generation_,a,b});
        bool available=false;
        for(unsigned attempt=0;attempt<1024&&n<9999999;++attempt){
            ++n;Bytes existing;auto result=files_.read(file_name(n),existing,MaxFile);
            if(result==ReadResult::Error)return fail("Cannot inspect save generation");
            if(result==ReadResult::Missing){available=true;break;}
        }
        if(!available)return fail("No free save generation; old files retained");
        auto data=encode(workbook,view,n);
        if(!files_.write(file_name(n),data,true))return fail("Workbook write/close failed; prior save retained");
        Bytes verified;
        if(files_.read(file_name(n),verified,MaxFile)!=ReadResult::Ok||verified!=data)return fail("Save verification failed; prior save retained");
        Bytes head{'S','4','7','H'};u32(head,n);checksum(head);
        const char* target=(n&1)?"HEAD1.S47":"HEAD0.S47";
        // Preserve whichever pointer identifies the newest valid prior save.
        if(valid_a&&(!valid_b||a>b))target="HEAD1.S47";
        else if(valid_b)target="HEAD0.S47";
        if(!files_.write(target,head,false))return fail("Save header failed; prior save retained");
        Bytes verified_head;
        if(files_.read(target,verified_head,64)!=ReadResult::Ok||verified_head!=head)return fail("Save header verification failed");
        generation_=n;error_[0]=0;return true;
    }catch(const std::bad_alloc&){return fail("Memory full during save; worksheet retained");}
     catch(const std::exception& e){return fail(e.what());}
}
LoadResult Storage::load(Workbook& workbook,ViewState& view){
    try{
        if(!files_.prepare()){fail("Storage unavailable");return LoadResult::Failed;}
        bool found=false,io_error=false;
        std::array<uint32_t,2> versions{head_generation(files_,"HEAD0.S47",found,io_error),head_generation(files_,"HEAD1.S47",found,io_error)};
        if(versions[0]<versions[1])std::swap(versions[0],versions[1]);
        bool invalid=false;
        for(auto n:versions){
            if(!n)continue;
            Bytes data;
            if(files_.read(file_name(n),data,MaxFile)!=ReadResult::Ok){invalid=true;continue;}
            try{
                ViewState candidate_view;auto records=decode(data,n,candidate_view);
                if(!workbook.replace(records)){fail(workbook.error());return LoadResult::Failed;}
                view=candidate_view;generation_=n;
                if(invalid)fail("Recovered previous valid save");else error_[0]=0;
                return LoadResult::Loaded;
            }catch(const std::runtime_error&){invalid=true;}
        }
        if(!found&&!io_error){error_[0]=0;return LoadResult::Empty;}
        fail("No valid save; current worksheet unchanged");return LoadResult::Failed;
    }catch(const std::bad_alloc&){fail("Memory full during load; worksheet retained");return LoadResult::Failed;}
     catch(const std::exception& e){fail(e.what());return LoadResult::Failed;}
}
}
