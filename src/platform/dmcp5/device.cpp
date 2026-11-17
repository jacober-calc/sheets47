// Original Sheets47 code, MIT license. Lifecycle structure follows official DMCP
// services and the documented Brainbox references; no GPL application code copied.
#include "app/application.hpp"
#include "ui/renderer.hpp"
#include "files.hpp"
#include "input.hpp"
#include "setup.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/time.h>
#include <cerrno>
#include "sdk.hpp"

namespace {
using namespace sheets47;
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
        auto files=std::make_unique<dmcp5::DeviceFiles>();auto storage=std::make_unique<Storage>(*files);
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
                reset_auto_off();int raw_key=key_pop();auto event=dmcp5::decode_key(raw_key);
                if(app->shifted&&event.key==Key::Exit){app->shifted=false;SET_ST(STAT_PGM_END);continue;}
                redraw|=app->handle(event);
                if(event.key==Key::Unmapped){
                    char message[80];std::snprintf(message,sizeof message,"Unmapped key %d; use FILE > SETUP",raw_key);app->notify(message);
                }
                if(app->take_setup_request()){
                    if(dmcp5::open_setup(*app))renderer->invalidate();
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
