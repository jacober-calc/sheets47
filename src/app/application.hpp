// Original Sheets47 code, MIT license.
#pragma once
#include "engine/workbook.hpp"
#include "storage/storage.hpp"
#include <array>

namespace sheets47 {
enum class Key { None, Up, Down, Left, Right, Enter, Backspace, Exit, Shift, Digit, Character,
    Add, Subtract, Multiply, Divide, Dot, Sign, Exponent, F1, F2, F3, F4, F5, F6 };
struct Event { Key key = Key::None; char character = 0; };
enum class Mode { Worksheet, Editor, Reference, Functions, Alphabet, GoTo, Help, Info, Confirm };
enum class Menu { Worksheet, Range, File, More };
class Application {
public:
    explicit Application(Storage&);
    bool start();
    bool handle(Event);
    bool save();
    bool suspend();
    void resumed();
    bool dirty() const { return workbook.generation()!=saved_generation_; }
    bool editing() const { return mode==Mode::Editor || mode==Mode::GoTo || mode==Mode::Alphabet; }
    bool alpha() const { return mode==Mode::Alphabet; }
    Cell edit_origin() const { return edit_cell_; }
    bool take_setup_request() { bool value=setup_request_;setup_request_=false;return value; }
    void notify(const char*);
    std::array<const char*,6> softkeys() const;
    Area selection() const { return Area::between(selecting?anchor:view.active,view.active); }

    Workbook workbook;
    ViewState view;
    Mode mode=Mode::Worksheet;
    Menu menu=Menu::Worksheet;
    bool selecting=false;
    Cell anchor{};
    std::string editor;
    size_t edit_cursor=0;
    unsigned function_index=0;
    unsigned alphabet_bank=0;
    bool shifted=false;
    char status[96]{};
    char memory_info[64]{};
    static constexpr std::array<const char*,9> functions{"SUM","AVERAGE","MIN","MAX","COUNT","IF","ROUND","ROUNDUP","ROUNDDOWN"};
private:
    Storage& storage_;
    uint32_t saved_generation_=0;
    bool setup_request_=false;
    std::vector<Record> clipboard_;
    Area copied_{};
    Cell edit_cell_{};
    bool has_clipboard_=false;
    Mode alphabet_return_=Mode::Editor;
    enum class Pending { None, New, Demo, Open, Clear, DeleteRow } pending_=Pending::None;
    void move(int,int);
    void begin_edit(bool formula=false);
    void append(const std::string&);
    void confirm(Pending,const char*);
    void execute(Pending);
    bool feedback(bool);
};
}
