// Original Sheets47 code, MIT license.
#include "application.hpp"
#include <algorithm>
#include <cstdio>

namespace sheets47 {
Application::Application(Storage& storage):storage_(storage) {}
void Application::notify(const char* value){std::snprintf(status,sizeof status,"%s",value);}
bool Application::start(){
    auto result=storage_.load(workbook,view);
    saved_generation_=workbook.generation();
    move(0,0);
    if(result==LoadResult::Loaded)notify(storage_.error()[0]?storage_.error():"Workbook opened");
    else if(result==LoadResult::Empty)notify("NEW: ENTER/EDIT to enter a cell");
    else notify(storage_.error());
    return result!=LoadResult::Failed;
}
bool Application::feedback(bool ok){notify(ok?"READY":workbook.error());return ok;}
bool Application::save(){
    if(storage_.save(workbook,view)){saved_generation_=workbook.generation();notify("SAVED /SHEETS47");return true;}
    notify(storage_.error());return false;
}
bool Application::suspend(){return !dirty() || save();}
bool Application::prepare_setup(){
    shifted=false;
    bool continue_unsaved=setup_without_save_;setup_without_save_=false;
    if(mode==Mode::Reference||mode==Mode::Functions||editing()){
        notify("Finish or cancel the edit before SETUP");return false;
    }
    if(dirty()&&!continue_unsaved&&!save()){
        confirm(Pending::Setup,"Save failed. SETUP can reset; continue unsaved?");return false;
    }
    return true;
}
void Application::resumed(){if(dirty())notify("UNSAVED: FILE > SAVE to retry");}
void Application::move(int dx,int dy){
    view.active.col=uint8_t(std::clamp(int(view.active.col)+dx,0,int(MaxCols)-1));
    view.active.row=uint16_t(std::clamp(int(view.active.row)+dy,0,int(MaxRows)-1));
    if(view.active.row<view.top)view.top=view.active.row;
    if(view.active.row>=view.top+6)view.top=view.active.row-5;
    if(view.active.col<view.left)view.left=view.active.col;
    if(view.active.col>=view.left+4)view.left=view.active.col-3;
}
void Application::begin_edit(bool formula){
    edit_cell_=view.active;
    editor=formula?"=":workbook.input(view.active);edit_cursor=editor.size();
    mode=Mode::Editor;notify("ENTER commits; EXIT cancels");
}
void Application::append(const std::string& text){
    if(editor.size()+text.size()>MaxInput){notify("Input limit: 143 characters");return;}
    editor.insert(edit_cursor,text);edit_cursor+=text.size();
}
void Application::confirm(Pending pending,const char* message){pending_=pending;mode=Mode::Confirm;notify(message);}
void Application::execute(Pending pending){
    if(pending==Pending::New){if(feedback(workbook.replace({}))){view={};notify("NEW WORKSHEET - unsaved");}}
    else if(pending==Pending::Demo){if(feedback(workbook.demo())){view={};view.active={1,3};notify("DEMO: edit B2 to change totals");}}
    else if(pending==Pending::Open){
        auto result=storage_.load(workbook,view);
        if(result==LoadResult::Loaded){saved_generation_=workbook.generation();notify(storage_.error()[0]?storage_.error():"Workbook opened");move(0,0);}
        else notify(result==LoadResult::Empty?"No saved workbook":storage_.error());
    }else if(pending==Pending::Clear)feedback(workbook.clear(selection()));
    else if(pending==Pending::DeleteRow)feedback(workbook.erase_row(view.active.row));
    else if(pending==Pending::Setup){setup_request_=true;setup_without_save_=true;}
    selecting=false;menu=Menu::Worksheet;
}
std::array<const char*,6> Application::softkeys() const {
    if(mode==Mode::Alphabet){
        static const std::array<const char*,6> banks[]={{"A","B","C","D","E","F"},{"G","H","I","J","K","L"},{"M","N","O","P","Q","R"},{"S","T","U","V","W","X"},{"Y","Z","SPACE","'","$",":"},{"=",",","^","<",">","%"}};
        return banks[alphabet_bank];
    }
    if(mode==Mode::Confirm)return {pending_==Pending::Setup?"CONTINUE":"YES","","","","","CANCEL"};
    if(mode==Mode::Help||mode==Mode::Info)return {"","","","","","BACK"};
    if(mode==Mode::Functions)return {"","","","","","BACK"};
    if(mode==Mode::Reference)return {"INSERT",":","$","","","BACK"};
    if(editing())return {"ABC","REF","FUNC","(",")","CANCEL"};
    if(menu==Menu::Range)return {"COPY","PASTE","FILLDN","INSROW","CLEAR","BACK"};
    if(menu==Menu::File)return {"SAVE","OPEN","NEW","DEMO","SETUP","BACK"};
    if(menu==Menu::More)return {"GO TO","INSROW","DELROW","HELP","INFO","BACK"};
    return {"EDIT","RANGE","FORM","FORMAT","FILE",">>"};
}
bool Application::handle(Event event){
    try{
        if(event.key==Key::None)return false;
        if(event.key==Key::Shift){shifted=!shifted;return true;}
        bool shifted_event=shifted;shifted=false;
        if(event.key==Key::Unmapped){notify("Key has no worksheet action; FILE > SETUP");return true;}
        if(shifted_event&&event.key==Key::Digit&&event.character=='0'){setup_request_=true;return true;}
        if(mode==Mode::Confirm){
            if(event.key==Key::F1){auto action=pending_;pending_=Pending::None;mode=Mode::Worksheet;execute(action);}
            else if(event.key==Key::Exit||event.key==Key::F6){pending_=Pending::None;mode=Mode::Worksheet;notify("Cancelled");}
            return true;
        }
        if(mode==Mode::Help||mode==Mode::Info){
            if(event.key==Key::Exit||event.key==Key::Enter||event.key==Key::F6)mode=Mode::Worksheet;
            return true;
        }
        if(mode==Mode::Alphabet){
            static const char* banks[]={"ABCDEF","GHIJKL","MNOPQR","STUVWX","YZ '$:","=,^<>%"};
            if(event.key==Key::Up||(event.key==Key::Digit&&event.character=='8'))alphabet_bank=(alphabet_bank+5)%6;
            else if(event.key==Key::Down||(event.key==Key::Digit&&event.character=='2'))alphabet_bank=(alphabet_bank+1)%6;
            else if(event.key>=Key::F1&&event.key<=Key::F6)append(std::string(1,banks[alphabet_bank][int(event.key)-int(Key::F1)]));
            else if(event.key==Key::Enter||event.key==Key::Exit)mode=alphabet_return_;
            else if(event.key==Key::Backspace){if(edit_cursor)editor.erase(--edit_cursor,1);}
            return true;
        }
        if(mode==Mode::Functions){
            if(event.key==Key::Up||(event.key==Key::Digit&&event.character=='8'))function_index=(function_index+functions.size()-1)%functions.size();
            else if(event.key==Key::Down||(event.key==Key::Digit&&event.character=='2'))function_index=(function_index+1)%functions.size();
            else if(event.key==Key::Enter){append(std::string(functions[function_index])+"(");mode=Mode::Editor;}
            else if(event.key==Key::Exit||event.key==Key::F6)mode=Mode::Editor;
            return true;
        }
        if(mode==Mode::Reference){
            if(event.key==Key::Enter||event.key==Key::F1){append(address(view.active));mode=Mode::Editor;}
            else if(event.key==Key::F2)append(":");
            else if(event.key==Key::F3)append("$");
            else if(event.key==Key::Exit||event.key==Key::F6)mode=Mode::Editor;
            else if(event.key==Key::Up||(event.key==Key::Digit&&event.character=='8'))move(0,-1);
            else if(event.key==Key::Down||(event.key==Key::Digit&&event.character=='2'))move(0,1);
            else if(event.key==Key::Left||(event.key==Key::Digit&&event.character=='4'))move(-1,0);
            else if(event.key==Key::Right||(event.key==Key::Digit&&event.character=='6'))move(1,0);
            return true;
        }
        if(editing()){
            if(event.key==Key::Exit||event.key==Key::F6){if(mode==Mode::Editor){view.active=edit_cell_;move(0,0);}mode=Mode::Worksheet;notify("Edit cancelled");return true;}
            if(event.key==Key::Enter){
                if(mode==Mode::GoTo){Cell target;if(parse_address(editor,target)){view.active=target;move(0,0);mode=Mode::Worksheet;menu=Menu::Worksheet;notify("READY");}else notify("Enter an address A1 to Z256");}
                else if(feedback(workbook.set(edit_cell_,editor))){mode=Mode::Worksheet;view.active=edit_cell_;move(0,0);}
                return true;
            }
            if(event.key==Key::Backspace){if(edit_cursor)editor.erase(--edit_cursor,1);return true;}
            if(event.key==Key::Left){if(edit_cursor)--edit_cursor;return true;}
            if(event.key==Key::Right){if(edit_cursor<editor.size())++edit_cursor;return true;}
            if(event.key==Key::F1){alphabet_return_=mode;mode=Mode::Alphabet;return true;}
            if(mode==Mode::Editor&&event.key==Key::F2){if(editor.empty())append("=");mode=Mode::Reference;notify("Move to cell; ENTER inserts reference");return true;}
            if(mode==Mode::Editor&&event.key==Key::F3){if(editor.empty())append("=");mode=Mode::Functions;return true;}
            if(event.key==Key::F4)append("(");else if(event.key==Key::F5)append(")");
            else if(event.key==Key::Character||event.key==Key::Digit)append(std::string(1,event.character));
            else if(event.key==Key::Add)append("+");else if(event.key==Key::Subtract)append("-");
            else if(event.key==Key::Multiply)append("*");else if(event.key==Key::Divide)append("/");
            else if(event.key==Key::Dot)append(".");else if(event.key==Key::Exponent)append("E");
            else if(event.key==Key::Sign)append("-");
            return true;
        }
        if(event.key==Key::Up||(event.key==Key::Digit&&event.character=='8')){move(0,-1);notify("READY");}
        else if(event.key==Key::Down||(event.key==Key::Digit&&event.character=='2')){move(0,1);notify("READY");}
        else if(event.key==Key::Left||(event.key==Key::Digit&&event.character=='4')){move(-1,0);notify("READY");}
        else if(event.key==Key::Right||(event.key==Key::Digit&&event.character=='6')){move(1,0);notify("READY");}
        else if(event.key==Key::Enter||(event.key==Key::Digit&&event.character=='5'))begin_edit();
        else if(event.key==Key::Exit){selecting=false;menu=Menu::Worksheet;notify("READY");}
        else if(event.key>=Key::F1&&event.key<=Key::F6){
            int f=int(event.key)-int(Key::F1);
            if(menu==Menu::Worksheet){
                if(f==0)begin_edit();
                else if(f==1){selecting=true;anchor=view.active;menu=Menu::Range;notify("Move to extend selection");}
                else if(f==2)begin_edit(true);
                else if(f==3){auto next=Format((unsigned(workbook.format(view.active))+1)%3);feedback(workbook.set_format(selection(),next));}
                else if(f==4)menu=Menu::File;else menu=Menu::More;
            }else if(menu==Menu::Range){
                if(f==0){auto area=selection();std::vector<Record> copy;for(const auto& r:workbook.records())if(r.cell.row>=area.first.row&&r.cell.row<=area.last.row&&r.cell.col>=area.first.col&&r.cell.col<=area.last.col)copy.push_back(r);clipboard_.swap(copy);copied_=area;has_clipboard_=true;selecting=false;notify("Copied; move then RANGE > PASTE");}
                else if(f==1){if(has_clipboard_)feedback(workbook.paste(clipboard_,copied_,view.active));else notify("Clipboard empty");selecting=false;}
                else if(f==2){feedback(workbook.fill_down(selection()));selecting=false;}
                else if(f==3)feedback(workbook.insert_row(view.active.row));
                else if(f==4)confirm(Pending::Clear,"Clear selected cells? YES / CANCEL");
                else {menu=Menu::Worksheet;selecting=false;}
            }else if(menu==Menu::File){
                if(f==0)save();
                else if(f==1){if(dirty())confirm(Pending::Open,"Discard unsaved edits and open?");else execute(Pending::Open);}
                else if(f==2){if(dirty())confirm(Pending::New,"Discard unsaved edits and create new?");else execute(Pending::New);}
                else if(f==3){if(dirty())confirm(Pending::Demo,"Replace unsaved sheet with demo?");else execute(Pending::Demo);}
                else if(f==4)setup_request_=true;else menu=Menu::Worksheet;
            }else {
                if(f==0){editor.clear();edit_cursor=0;mode=Mode::GoTo;notify("Enter address, e.g. A12; ENTER");}
                else if(f==1)feedback(workbook.insert_row(view.active.row));
                else if(f==2)confirm(Pending::DeleteRow,"Delete this row and shift cells up?");
                else if(f==3)mode=Mode::Help;else if(f==4)mode=Mode::Info;else menu=Menu::Worksheet;
            }
        }else return false;
        return true;
    }catch(const std::bad_alloc&){notify("Memory full; cancel edit or save workbook");return true;}
}
}
