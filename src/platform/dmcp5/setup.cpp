// Original Sheets47 code, MIT license.
#include "setup.hpp"
#include "sdk.hpp"
namespace sheets47::dmcp5 {
namespace {
const uint8_t settings_items[]={MI_BASE_SETUP,MI_MSC,MI_SYSTEM_ENTER,MI_PGM_INFO,MI_EXIT,0};
const smenu_t settings_menu={"sheets47 0.1.0",settings_items,nullptr,nullptr};
}
bool open_setup(Application& app) {
    if(!app.prepare_setup())return false;
    // Disk operations have returned and every handle is closed. Storage failure
    // can require confirmation, but must never lock the user out of DMCP.
    SET_ST(STAT_MENU);handle_menu(&settings_menu,MENU_RESET,0);CLR_ST(STAT_MENU);
    wait_for_key_release(-1);
    app.notify(app.dirty()?"UNSAVED: FILE > SAVE to retry":"READY");
    return true;
}
}
