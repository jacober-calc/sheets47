// Original Sheets47 code, MIT license. Real platform adapters, stub firmware I/O.
#include "platform/dmcp5/files.hpp"
#include "platform/dmcp5/input.hpp"
#include "platform/dmcp5/setup.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include "platform/dmcp5/sdk.hpp"

extern "C" { sys_sdb_t sheets47_test_sdb{}; }
namespace {
using namespace sheets47;
unsigned checks=0;
void require(bool ok,const char* message){
    ++checks;if(!ok){std::fprintf(stderr,"FAIL DMCP5: %s\n",message);std::exit(1);}
}
struct Firmware {
    bool ready=true,refuse_write=false,directory=false,short_write=false;
    int write_enabled=0,dir_result=0;
    FRESULT open_error=FR_OK,write_error=FR_OK,close_error=FR_OK;
    FIL* opened=nullptr;
    BYTE mode=0;
    std::string filename;
    std::map<std::string,std::vector<uint8_t>> data;
    unsigned menu_calls=0,release_waits=0,directory_checks=0,writes=0;
} fw;
void press(Application& app,int code){app.handle(dmcp5::decode_key(code));}
void shifted_zero(Application& app,bool released=true){
    press(app,KEY_SHIFT);if(released)press(app,0);press(app,KEY_0);
    require(!app.shifted,"SHIFT consumed by zero");
    require(app.take_setup_request(),"SDK SHIFT/zero events request setup");
}
}
extern "C" {
int sys_disk_ok(){return fw.ready;}
int sys_is_disk_write_enable(){return fw.write_enabled;}
int sys_disk_write_enable(int value){
    int previous=fw.write_enabled;
    if(!value||!fw.refuse_write)fw.write_enabled=value;
    return previous;
}
int check_create_dir(const char* name){
    ++fw.directory_checks;
    require(fw.opened==nullptr,"directory checked without an open file");
    require(fw.write_enabled!=0,"directory creation has firmware write access");
    require(std::string(name)=="/SHEETS47","only root/SHEETS47 is prepared");
    if(fw.dir_result)return fw.dir_result;
    fw.directory=true;return 0;
}
FRESULT f_open(FIL* file,const TCHAR* name,BYTE mode){
    require(fw.opened==nullptr,"at most one SDK file handle");
    require(std::string(name).starts_with("/SHEETS47/"),"file path confined to SHEETS47");
    if(fw.open_error!=FR_OK)return fw.open_error;
    if(!fw.directory)return FR_NO_PATH;
    if(mode&FA_WRITE){
        require(fw.write_enabled!=0,"file create/open has firmware write access");
        if((mode&FA_CREATE_NEW)&&fw.data.count(name))return FR_EXIST;
        fw.data[name]={};
    }else if(!fw.data.count(name))return FR_NO_FILE;
    fw.opened=file;fw.filename=name;fw.mode=mode;
    *file={};file->obj.objsize=fw.data[name].size();return FR_OK;
}
FRESULT f_close(FIL* file){
    require(file==fw.opened,"close the actual SDK handle");
    if(fw.mode&FA_WRITE)require(fw.write_enabled!=0,"flush/close before write protection restored");
    fw.opened=nullptr;return fw.close_error;
}
FRESULT f_write(FIL* file,const void* source,UINT count,UINT* written){
    ++fw.writes;
    require(file==fw.opened&&fw.write_enabled&&(fw.mode&FA_WRITE),"write owns a writable SDK handle");
    *written=fw.short_write?count/2:count;
    if(fw.write_error!=FR_OK){*written=0;return fw.write_error;}
    auto bytes=static_cast<const uint8_t*>(source);
    fw.data[fw.filename].assign(bytes,bytes+*written);return FR_OK;
}
FRESULT f_read(FIL* file,void* target,UINT count,UINT* read){
    require(file==fw.opened&&!(fw.mode&FA_WRITE),"read owns the SDK handle");
    auto& data=fw.data[fw.filename];*read=std::min<size_t>(count,data.size());
    if(*read)std::memcpy(target,data.data(),*read);
    return FR_OK;
}
int handle_menu(const smenu_t* menu,int action,int){
    ++fw.menu_calls;
    require(ST(STAT_MENU),"DMCP menu state set before menu entry");
    require(fw.opened==nullptr&&fw.write_enabled==0,"menu/USB entry has no handle or write permission");
    require(action==MENU_RESET,"menu starts at its root");
    bool usb=false,system=false;
    for(const uint8_t* item=menu->items;*item;++item){usb|=*item==MI_MSC;system|=*item==MI_SYSTEM_ENTER;}
    require(usb&&system,"DMCP system and USB paths are offered");return 0;
}
void wait_for_key_release(int timeout){++fw.release_waits;require(timeout==-1,"menu waits for triggering key release");}
}

unsigned dmcp5_tests(){
    using namespace sheets47;
    fw={};dmcp5::DeviceFiles files;
    require(files.prepare(),"prepare enables normally disabled firmware writes");
    require(fw.directory&&fw.write_enabled==0,"prepare creates directory and restores protection");
    const std::vector<uint8_t> bytes{1,2,3,4};
    require(files.write("TEST.S47",bytes,true),"write succeeds from protected idle state");
    require(fw.write_enabled==0&&!fw.opened,"successful write restores protection and closes");
    require(!files.write("TEST.S47",bytes,true)&&fw.data["/SHEETS47/TEST.S47"]==bytes,"exclusive create preserves existing bytes");
    require(fw.write_enabled==0&&!fw.opened,"open failure restores protection");
    fw.short_write=true;require(!files.write("SHORT.S47",bytes,true),"short write reported");fw.short_write=false;
    require(fw.write_enabled==0&&!fw.opened,"short write closes before restoring protection");
    fw.close_error=FR_DISK_ERR;require(!files.write("CLOSE.S47",bytes,true),"failed close reported");fw.close_error=FR_OK;
    require(fw.write_enabled==0&&!fw.opened,"failed close restores protection");
    fw.write_error=FR_DENIED;require(!files.write("FULL.S47",bytes,true),"full/denied disk reported");fw.write_error=FR_OK;
    require(fw.write_enabled==0&&!fw.opened,"write error restores protection");
    fw.open_error=FR_DISK_ERR;require(!files.write("OPEN.S47",bytes,true),"open disk error reported");fw.open_error=FR_OK;
    require(fw.write_enabled==0,"open disk error restores protection");
    fw.dir_result=7;require(!files.prepare(),"directory failure reported");fw.dir_result=0;
    require(fw.write_enabled==0&&std::strstr(files.error(),"/SHEETS47"),"directory error retains cause and protection");
    fw.refuse_write=true;require(!files.prepare()&&!files.write("DENIED.S47",bytes,true),"firmware refusal respected");fw.refuse_write=false;
    require(fw.write_enabled==0,"refused access retains protection");
    fw.write_enabled=1;require(files.prepare()&&files.write("PREV.S47",bytes,true),"already-enabled caller can write");
    require(fw.write_enabled==1,"caller write state restored exactly");fw.write_enabled=0;
    std::vector<uint8_t> loaded;
    require(files.read("TEST.S47",loaded,64)==ReadResult::Ok&&loaded==bytes,"device adapter reads saved bytes");
    require(files.read("TEST.S47",loaded,2)==ReadResult::Error&&!fw.opened,"oversized read closes handle");
    require(files.read("MISSING.S47",loaded,64)==ReadResult::Missing,"missing file distinct from disk error");
    fw.ready=false;require(!files.prepare()&&!files.write("NOTREADY.S47",bytes,true),"unavailable disk rejected");fw.ready=true;

    Storage storage(files);Application app(storage);require(app.start(),"start against protected firmware stub");
    shifted_zero(app);require(dmcp5::open_setup(app),"released SHIFT then zero opens setup");
    require(!ST(STAT_MENU)&&fw.release_waits==1,"return clears menu state and waits for release");
    shifted_zero(app,false);require(dmcp5::open_setup(app),"SHIFT then zero without intermediate release opens setup");
    press(app,KEY_F5);press(app,KEY_F4);require(app.dirty(),"demo is unsaved");
    press(app,KEY_F5);press(app,KEY_F1);require(!app.dirty(),"FILE/SAVE succeeds with initially protected firmware disk");
    require(app.workbook.set({9,0},"EDIT"),"dirty fixture for setup autosave");
    press(app,KEY_F5);require(app.take_setup_request(),"FILE/SETUP requests same path");
    require(dmcp5::open_setup(app)&&!app.dirty(),"FILE/SETUP saves dirty work and opens menu");
    require(fw.data.count("/SHEETS47/HEAD1.S47"),"device save publishes first generation");
    Workbook restored;ViewState view;Storage reader(files);
    require(reader.load(restored,view)==LoadResult::Loaded&&restored.display({5,3})=="18.50","real device adapter save/load roundtrip");
    auto writes=fw.writes;shifted_zero(app);require(dmcp5::open_setup(app)&&fw.writes==writes,"clean setup does not write workbook");

    require(app.workbook.set({1,1},"5"),"dirty workbook before failed setup save");
    fw.close_error=FR_DISK_ERR;require(!app.save()&&app.dirty(),"failed device close keeps worksheet unsaved");fw.close_error=FR_OK;
    require(fw.write_enabled==0&&!fw.opened,"failed Save restores protection before any menu");
    require(reader.load(restored,view)==LoadResult::Loaded&&restored.display({5,3})=="18.50","failed device Save leaves prior workbook recoverable");
    fw.ready=false;
    auto menus=fw.menu_calls;shifted_zero(app);
    require(!dmcp5::open_setup(app)&&app.mode==Mode::Confirm,"failed setup save asks to continue unsaved");
    require(fw.menu_calls==menus&&std::string(app.softkeys()[0])=="CONTINUE","failed save has explicit continue/cancel choice");
    press(app,KEY_F6);require(app.mode==Mode::Worksheet&&!app.take_setup_request(),"cancel keeps user in worksheet");
    require(app.workbook.display({5,3})=="23.50"&&app.dirty(),"cancel preserves unsaved workbook");
    press(app,KEY_F5);press(app,KEY_F5);require(app.take_setup_request(),"FILE/SETUP remains reachable with broken storage");
    require(!dmcp5::open_setup(app),"FILE/SETUP warns on failed save");
    press(app,KEY_F1);require(app.take_setup_request(),"continue unsaved retries menu entry");
    auto directory_checks=fw.directory_checks;
    require(dmcp5::open_setup(app)&&fw.menu_calls==menus+1,"DMCP accessible even while disk unavailable");
    require(app.dirty()&&app.workbook.display({5,3})=="23.50","return retains unsaved data");
    require(fw.directory_checks==directory_checks&&std::strstr(app.status,"UNSAVED"),"continue does not loop on save and return reports unsaved");
    shifted_zero(app);require(!dmcp5::open_setup(app),"next setup tries save again rather than retaining bypass");press(app,KEY_F6);
    fw.ready=true;require(app.save()&&!app.dirty(),"save works after disk becomes available");
    require(reader.load(restored,view)==LoadResult::Loaded&&restored.display({5,3})=="23.50","latest device save reopens edited result");
    press(app,KEY_F1);require(app.editing(),"draft fixture");shifted_zero(app);
    require(!dmcp5::open_setup(app)&&app.editing(),"setup preserves draft and requests finish/cancel");press(app,KEY_EXIT);
    press(app,KEY_SHIFT);press(app,0);press(app,KEY_DOUBLE_RELEASE);
    require(app.shifted,"release events do not consume latched SHIFT");
    press(app,KEY_RUN);require(!app.shifted&&std::strstr(app.status,"FILE > SETUP"),"unmapped press clears SHIFT and offers working menu path");
    require(dmcp5::decode_key(KEY_0).key==Key::Digit&&dmcp5::decode_key(KEY_0).character=='0',"SDK zero decodes as zero");
    const int digit_codes[]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
    for(unsigned i=0;i<10;++i){
        auto event=dmcp5::decode_key(digit_codes[i]);
        require(event.key==Key::Digit&&event.character=='0'+i,"SDK digit mapping retained");
    }
    std::printf("PASS %u DMCP5 adapter checks: protected writes, save/load, SHIFT+0, FILE/SETUP, failed-save escape\n",checks);
    return checks;
}
