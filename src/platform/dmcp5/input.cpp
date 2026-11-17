// Original Sheets47 code, MIT license. Key releases preserve a latched SHIFT.
#include "input.hpp"
#include "sdk.hpp"
namespace sheets47::dmcp5 {
Event decode_key(int key) {
    if(key<=0||key==KEY_DOUBLE_RELEASE)return {};
    if(key>=KEY_F1&&key<=KEY_F6)return {Key(int(Key::F1)+key-KEY_F1),0};
    static constexpr int digits[]={KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};
    for(unsigned i=0;i<10;++i)if(key==digits[i])return {Key::Digit,char('0'+i)};
    switch(key){
        case KEY_UP:return {Key::Up};case KEY_DOWN:return {Key::Down};
        case KEY_SWAP:return {Key::Left};case KEY_RDN:return {Key::Right};
        case KEY_ENTER:return {Key::Enter};case KEY_BSP:return {Key::Backspace};case KEY_EXIT:return {Key::Exit};
        case KEY_SHIFT:return {Key::Shift};case KEY_ADD:return {Key::Add};case KEY_SUB:return {Key::Subtract};
        case KEY_MUL:return {Key::Multiply};case KEY_DIV:return {Key::Divide};case KEY_DOT:return {Key::Dot};
        case KEY_CHS:return {Key::Sign};case KEY_E:return {Key::Exponent};default:return {Key::Unmapped};
    }
}
}
