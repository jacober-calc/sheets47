// Original Sheets47 code, MIT license. Lifecycle structure follows official DMCP
// services and the documented Brainbox references; no GPL application code copied.
#include "app/application.hpp"
#include "ui/renderer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/time.h>
#include <cerrno>
extern "C" {
#include <dmcp.h>
}

namespace {
using namespace sheets47;
class DeviceFiles final:public FilePort {
    static std::string path(const std::string& name){return std::string(DataDirectory)+"/"+name;}
public:
    bool prepare() override{return sys_disk_ok()&&sys_is_disk_write_enable()&&check_create_dir(DataDirectory)==0;}
    ReadResult read(const std::string& name,std::vector<uint8_t>& data,size_t max) override{
        FIL file;auto result=f_open(&file,path(name).c_str(),FA_READ);
        if(result==FR_NO_FILE||result==FR_NO_PATH)return ReadResult::Missing;
        if(result!=FR_OK)return ReadResult::Error;
        try{
            auto length=f_size(&file);
            if(length>max){f_close(&file);return ReadResult::Error;}
            data.resize(length);UINT count=0;
            auto read_result=f_read(&file,data.data(),UINT(data.size()),&count);
            auto close_result=f_close(&file);
            return read_result==FR_OK&&close_result==FR_OK&&count==data.size()?ReadResult::Ok:ReadResult::Error;
        }catch(...){f_close(&file);throw;}
    }
    bool write(const std::string& name,const std::vector<uint8_t>& data,bool create_new) override{
        if(!sys_disk_ok()||!sys_is_disk_write_enable())return false;
        FIL file;if(f_open(&file,path(name).c_str(),FA_WRITE|(create_new?FA_CREATE_NEW:FA_CREATE_ALWAYS))!=FR_OK)return false;
        UINT count=0;auto result=f_write(&file,data.data(),UINT(data.size()),&count);
        auto closed=f_close(&file);
        return result==FR_OK&&count==data.size()&&closed==FR_OK;
    }
};
class DeviceDisplay final:public DisplayPort {
public:
    void row(unsigned y,const uint8_t* pixels) override{
        for(unsigned x=0;x<400;){
            bool black=pixels[x/8]&(0x80u>>(x%8));unsigned end=x+1;
            while(end<400&&bool(pixels[end/8]&(0x80u>>(end%8)))==black)++end;
            lcd_fill_rect(x,y,end-x,1,black?1:0);x=end;
        }
    }
    void refresh(unsigned first,unsigned count) override{lcd_refresh_lines(int(first),int(count));lcd_refresh_wait();}
};
int app_menu(uint8_t){return MRET_UNIMPL;}
const char* app_menu_label(uint8_t,char*,int){return nullptr;}
const uint8_t settings_items[]={MI_BASE_SETUP,MI_MSC,MI_SYSTEM_ENTER,MI_PGM_INFO,MI_EXIT,0};
const smenu_t settings_menu={"sheets47 0.1.0",settings_items,nullptr,nullptr};

Event decode(int key){
    if(key>=KEY_F1&&key<=KEY_F6)return {Key(int(Key::F1)+key-KEY_F1),0};
    static constexpr int digits[]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
    for(unsigned i=0;i<10;++i)if(key==digits[i])return {Key::Digit,char('0'+i)};
    switch(key){
        case KEY_UP:return {Key::Up};case KEY_DOWN:return {Key::Down};
        case KEY_SWAP:return {Key::Left};case KEY_RDN:return {Key::Right};
        case KEY_ENTER:return {Key::Enter};case KEY_BSP:return {Key::Backspace};case KEY_EXIT:return {Key::Exit};
        case KEY_SHIFT:return {Key::Shift};case KEY_ADD:return {Key::Add};case KEY_SUB:return {Key::Subtract};
        case KEY_MUL:return {Key::Multiply};case KEY_DIV:return {Key::Divide};case KEY_DOT:return {Key::Dot};
        case KEY_CHS:return {Key::Sign};case KEY_E:return {Key::Exponent};default:return {};
    }
}
[[noreturn]] void fatal(const char* message){
    lcd_clear_buf();disp_stat_t display=*t20;display.inv=0;display.bgfill=1;display.lnfill=0;display.newln=0;
    lcd_setXY(&display,4,20);lcd_writeText(&display,"sheets47 0.1.0");
    lcd_setXY(&display,4,60);lcd_writeText(&display,message);
    lcd_setXY(&display,4,100);lcd_writeText(&display,"ENTER: DMCP menu");
    lcd_refresh();lcd_refresh_wait();
    for(;;){CLR_ST(STAT_RUNNING);sys_sleep();SET_ST(STAT_RUNNING);if(!key_empty()&&key_pop()==KEY_ENTER){set_reset_magic(RUN_DMCP_MAGIC);sys_reset();}}
}
}

extern "C" [[noreturn]] void _exit(int){fatal("Runtime stopped; restart from DMCP");}
extern "C" {
void* __dso_handle = &__dso_handle;
void _init() {}
void _fini() {}
struct _reent;
void __wrap__free_r(struct _reent*,void* p){std::free(p);}
int _gettimeofday(struct timeval* result,void*){
    tm_t time;dt_t date;rtc_read(&time,&date);
    if(!result||date.year<1970||date.month<1||date.month>12||date.day<1||date.day>31){errno=EINVAL;return -1;}
    result->tv_sec=static_cast<time_t>(julian_day(&date)-2440588)*86400+time.hour*3600+time.min*60+time.sec;
    result->tv_usec=time.csec*10000;return 0;
}
}
extern "C" void program_main(){
    using namespace sheets47;
    try{
        run_menu_item_app=app_menu;menu_line_str_app=app_menu_label;
        switch_usb_powered_freq();
        // No application clock, animation or blink timer is started.
        CLR_ST(STAT_CLK_WKUP_ENABLE|STAT_CLK_WKUP_SECONDS|STAT_CLK_WKUP_FLAG);
        auto files=std::make_unique<DeviceFiles>();auto storage=std::make_unique<Storage>(*files);
        auto app=std::make_unique<Application>(*storage);auto renderer=std::make_unique<Renderer>();DeviceDisplay display;
        if(ST(STAT_CLEAN_RESET)){CLR_ST(STAT_CLEAN_RESET);app->notify("Clean start; FILE > OPEN restores saved work");}
        else app->start();
        std::snprintf(app->memory_info,sizeof app->memory_info,"HEAP %dK / LARGEST %dK",sys_free_mem()/1024,sys_largest_free_mem()/1024);
        renderer->draw(*app);renderer->present(display,true);
        reset_auto_off();
        for(;;){
            if(ST(STAT_PGM_END)||ST(STAT_SUSPENDED)){
                if(!ST(STAT_SUSPENDED)){
                    app->suspend();lcd_refresh_wait();LCD_power_off(0);SET_ST(STAT_SUSPENDED|STAT_OFF);
                }
                CLR_ST(STAT_RUNNING);sys_sleep();continue;
            }
            if(key_empty()){
                CLR_ST(STAT_RUNNING);sys_sleep();
                if(ST(STAT_PGM_END)||ST(STAT_SUSPENDED))continue;
            }
            SET_ST(STAT_RUNNING);
            bool redraw=false;
            if(ST(STAT_OFF)){
                LCD_power_on();rtc_wakeup_delay();CLR_ST(STAT_OFF);app->resumed();renderer->invalidate();redraw=true;
            }
            if(ST(STAT_POWER_CHANGE)){switch_usb_powered_freq();CLR_ST(STAT_POWER_CHANGE);}
            if(!key_empty()){
                reset_auto_off();auto event=decode(key_pop());
                if(app->shifted&&event.key==Key::Exit){app->shifted=false;SET_ST(STAT_PGM_END);continue;}
                redraw|=app->handle(event);
                if(app->take_setup_request()){
                    if(app->mode==Mode::Reference||app->mode==Mode::Functions||app->editing())app->notify("Finish or cancel the edit before SETUP");
                    else if(!app->dirty()||app->save()){
                        // All file handles are closed before USB/system menus.
                        SET_ST(STAT_MENU);handle_menu(&settings_menu,MENU_RESET,0);CLR_ST(STAT_MENU);
                        wait_for_key_release(-1);renderer->invalidate();app->notify("READY");
                    }
                    redraw=true;
                }
            }
            if(redraw){
                std::snprintf(app->memory_info,sizeof app->memory_info,"HEAP %dK / LARGEST %dK",sys_free_mem()/1024,sys_largest_free_mem()/1024);
                renderer->draw(*app);renderer->present(display);
            }
        }
    }catch(const std::bad_alloc&){fatal("Not enough free memory");}
     catch(...){fatal("Startup failed; saved files retained");}
}
