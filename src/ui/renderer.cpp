// Original Sheets47 code, MIT license. Original compact 5x7 glyph definitions.
#include "renderer.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace sheets47 {
namespace {
struct Glyph{char c;uint8_t columns[5];};
constexpr Glyph glyphs[]={
 {' ',{0,0,0,0,0}},{'!',{0,0,95,0,0}},{'"',{0,7,0,7,0}},{'#',{20,127,20,127,20}},
 {'$',{36,42,127,42,18}},{'%',{35,19,8,100,98}},{'&',{54,73,85,34,80}},{'\'',{0,0,7,0,0}},
 {'(',{0,28,34,65,0}},{')',{0,65,34,28,0}},{'*',{20,8,62,8,20}},{'+',{8,8,62,8,8}},
 {',',{0,80,48,0,0}},{'-',{8,8,8,8,8}},{'.',{0,96,96,0,0}},{'/',{32,16,8,4,2}},
 {'0',{62,81,73,69,62}},{'1',{0,66,127,64,0}},{'2',{66,97,81,73,70}},{'3',{33,65,69,75,49}},
 {'4',{24,20,18,127,16}},{'5',{39,69,69,69,57}},{'6',{60,74,73,73,48}},{'7',{1,113,9,5,3}},
 {'8',{54,73,73,73,54}},{'9',{6,73,73,41,30}},{':',{0,54,54,0,0}},{';',{0,86,54,0,0}},
 {'<',{8,20,34,65,0}},{'=',{20,20,20,20,20}},{'>',{0,65,34,20,8}},{'?',{2,1,81,9,6}},
 {'@',{50,73,121,65,62}},{'A',{126,17,17,17,126}},{'B',{127,73,73,73,54}},{'C',{62,65,65,65,34}},
 {'D',{127,65,65,34,28}},{'E',{127,73,73,73,65}},{'F',{127,9,9,9,1}},{'G',{62,65,73,73,122}},
 {'H',{127,8,8,8,127}},{'I',{0,65,127,65,0}},{'J',{32,64,65,63,1}},{'K',{127,8,20,34,65}},
 {'L',{127,64,64,64,64}},{'M',{127,2,12,2,127}},{'N',{127,4,8,16,127}},{'O',{62,65,65,65,62}},
 {'P',{127,9,9,9,6}},{'Q',{62,65,81,33,94}},{'R',{127,9,25,41,70}},{'S',{38,73,73,73,50}},
 {'T',{1,1,127,1,1}},{'U',{63,64,64,64,63}},{'V',{31,32,64,32,31}},{'W',{63,64,56,64,63}},
 {'X',{99,20,8,20,99}},{'Y',{7,8,112,8,7}},{'Z',{97,81,73,69,67}},{'[',{0,127,65,65,0}},
 {'\\',{2,4,8,16,32}},{']',{0,65,65,127,0}},{'^',{4,2,1,2,4}},{'_', {64,64,64,64,64}},
 {'|',{0,0,127,0,0}},{'~',{8,4,8,16,8}}
};
const uint8_t* glyph(char c){
    c=char(std::toupper(static_cast<unsigned char>(c)));
    for(const auto& g:glyphs)if(g.c==c)return g.columns;
    for(const auto& g:glyphs)if(g.c=='?')return g.columns;
    return glyphs[0].columns;
}
constexpr int edges[]={0,28,152,222,304,390};
}
void Canvas::pixel(int x,int y,bool black){
    if(x<0||x>=400||y<0||y>=240)return;
    auto& p=pixels[size_t(y)*50+unsigned(x)/8];uint8_t mask=uint8_t(0x80u>>(x%8));
    if(black)p|=mask;else p&=uint8_t(~mask);
}
void Canvas::rect(int x,int y,int w,int h,bool black){
    for(int yy=std::max(0,y);yy<std::min(240,y+h);++yy)
        for(int xx=std::max(0,x);xx<std::min(400,x+w);++xx)pixel(xx,yy,black);
}
void Canvas::outline(int x,int y,int w,int h,int weight){rect(x,y,w,weight);rect(x,y+h-weight,w,weight);rect(x,y,weight,h);rect(x+w-weight,y,weight,h);}
void Canvas::text(int x,int y,const std::string& text,int sx,int sy,bool inverted,int max_width){
    int limit=std::min(400,x+max_width),origin=x;
    for(char c:text){
        if(x+5*sx>limit)break;
        auto g=glyph(c);
        for(int col=0;col<5;++col)for(int row=0;row<7;++row)if(g[col]&(1u<<row))rect(x+col*sx,y+row*sy,sx,sy,!inverted);
        x+=6*sx;
    }
    (void)origin;
}
void Renderer::worksheet(const Application& app){
    auto& c=canvas;
    c.rect(0,0,400,19);
    c.text(4,2,"SHEETS47",2,2,true,120);
    c.text(226,3,std::string("WORKBOOK")+(app.dirty()?" *":""),1,2,true,168);
    auto cell=app.mode==Mode::Editor||app.mode==Mode::Alphabet?app.edit_origin():app.view.active;
    c.outline(1,22,47,20);
    c.text(5,25,address(cell),1,2,false,42);
    c.text(57,25,app.mode==Mode::Reference?app.editor:app.workbook.input(cell),1,2,false,337);
    c.rect(0,45,400,1);
    for(unsigned i=0;i<4;++i){
        unsigned column=app.view.left+i;
        if(column>=MaxCols)continue;
        bool active=column==app.view.active.col;
        if(active)c.rect(edges[i+1]+1,46,edges[i+2]-edges[i+1]-1,17);
        c.text((edges[i+1]+edges[i+2])/2-3,48,std::string(1,char('A'+column)),1,2,active);
    }
    c.rect(0,63,391,1);
    Area selected=app.selection();
    for(unsigned i=0;i<6;++i){
        unsigned row=app.view.top+i;if(row>=MaxRows)continue;
        int top=64+int(i)*21;
        bool active=row==app.view.active.row;
        if(active)c.rect(0,top,28,21);
        char number[6];std::snprintf(number,sizeof number,"%u",row+1);c.text(2,top+3,number,1,2,active,26);
        for(unsigned j=0;j<4;++j){
            unsigned col=app.view.left+j;if(col>=MaxCols)continue;
            Cell value{uint16_t(row),uint8_t(col)};
            if(app.selecting&&row>=selected.first.row&&row<=selected.last.row&&col>=selected.first.col&&col<=selected.last.col)
                for(int yy=top+3;yy<top+19;yy+=4)for(int xx=edges[j+1]+3;xx<edges[j+2]-2;xx+=4)c.pixel(xx,yy);
            auto content=app.workbook.display(value);
            int width=edges[j+2]-edges[j+1]-8;
            int scale=content.size()*12<=size_t(width)?2:1;
            if(content.size()*6>size_t(width)){content.resize(unsigned(width)/6);if(content.size()>2)content.replace(content.size()-2,2,"..");}
            int x=edges[j+1]+4;
            if(app.workbook.numeric(value))x=edges[j+2]-4-int(content.size())*6*scale;
            c.text(x,top+3,content,scale,2,false,width);
        }
        c.rect(0,top+20,391,1);
    }
    for(int edge:edges)c.rect(edge,46,1,144);
    if(app.selecting){
        int left=std::max(int(selected.first.col),int(app.view.left))-int(app.view.left);
        int right=std::min(int(selected.last.col),int(app.view.left)+3)-int(app.view.left);
        int top=std::max(int(selected.first.row),int(app.view.top))-int(app.view.top);
        int bottom=std::min(int(selected.last.row),int(app.view.top)+5)-int(app.view.top);
        c.outline(edges[left+1],64+top*21,edges[right+2]-edges[left+1]+1,(bottom-top+1)*21,2);
    }
    unsigned active_col=app.view.active.col-app.view.left,active_row=app.view.active.row-app.view.top;
    if(active_col<4&&active_row<6){int x=edges[active_col+1],y=64+int(active_row)*21;c.outline(x,y,edges[active_col+2]-x+1,21,2);c.rect(edges[active_col+2]-3,y+17,4,4);}
    c.rect(392,64,1,126);c.rect(395,64+int(app.view.top)*100/int(MaxRows-6),3,24);
    std::string status=app.status;
    c.text(3,192,status.substr(0,44),1,1,false,266);
    if(status.size()>44)c.text(3,200,status.substr(44),1,1,false,390);
    c.rect(280,196,109,1);
    c.rect(280+std::min(unsigned(app.view.left),22u)*90/22,194,17,5);
    c.rect(276,195,1,3);c.pixel(275,196);
    c.rect(393,195,1,3);c.pixel(394,196);
}
void Renderer::draw(const Application& app){
    canvas.clear();worksheet(app);auto& c=canvas;
    if(app.mode==Mode::Editor||app.mode==Mode::Alphabet||app.mode==Mode::GoTo){
        c.rect(0,46,400,145,false);c.outline(1,47,398,142);
        auto label=app.mode==Mode::GoTo?"GO TO CELL":app.mode==Mode::Alphabet?"LETTERS: UP/DN BANK; ENTER BACK":"EDIT "+address(app.edit_origin());
        c.text(8,53,label,1,2);
        std::string draft=app.editor;
        draft.insert(std::min(app.edit_cursor,draft.size()),"_");
        for(unsigned row=0;row<5;++row)c.text(8,75+int(row)*18,draft.substr(std::min(size_t(row*31),draft.size()),31),2,2,false,384);
        c.text(8,169,app.mode==Mode::Alphabet?"F1-F6 INSERT; BSP ERASES":"ENTER SAVE CELL / EXIT CANCEL",1,2,false,384);
    }else if(app.mode==Mode::Functions){
        c.rect(0,46,400,145,false);c.text(5,48,"FUNCTIONS - ENTER INSERTS",1,2);
        for(unsigned i=0;i<9;++i){int x=i<5?6:204,y=69+int(i%5)*23;bool selected=i==app.function_index;
            if(selected)c.rect(x-2,y-1,186,18);
            c.text(x,y,app.functions[i],1,2,selected);}
    }else if(app.mode==Mode::Help||app.mode==Mode::Info){
        c.rect(0,46,400,145,false);
        const char* help[]={"NAV: 8/2/4/6 MOVE; ENTER EDIT","EDIT: F1 ABC; F2 REF; F3 FUNC","ABC: UP/DN BANK; F1-F6 LETTERS","FORM: =; REF INSERTS A1 ADDRESS","RANGE: MOVE THEN COPY OR FILLDN","FILE: SAVE / OPEN / NEW / DEMO","SHIFT+0 SETUP; SHIFT+EXIT OFF"};
        if(app.mode==Mode::Help)for(unsigned i=0;i<7;++i)c.text(5,49+int(i)*20,help[i],1,2,false,390);
        else {c.text(5,50,"SHEETS47 0.1.0 / SPREADER",1,2);c.text(5,73,"A1:Z256 / 256 POPULATED CELLS",1,2);c.text(5,96,"SAVE: /SHEETS47",1,2);c.text(5,119,app.memory_info,1,2);c.text(5,142,"IDLE: SLEEP / CHANGED ROWS ONLY",1,2);c.text(5,165,"HARDWARE TESTING REQUIRED",1,2);}
    }else if(app.mode==Mode::Confirm){
        c.rect(20,80,360,90,false);c.outline(20,80,360,90,2);c.text(30,92,"CONFIRM",2,2);
        std::string message=app.status;c.text(30,117,message.substr(0,54),1,2,false,340);c.text(30,139,message.size()>54?message.substr(54):"F1 YES / F6 CANCEL",1,2,false,340);
    }
    if(app.shifted)c.text(362,25,"SHIFT",1,1);
    auto keys=app.softkeys();
    for(int i=0;i<6;++i){int x=2+i*66;c.rect(x,210,62,28);std::string label=keys[i];c.text(x+(62-int(label.size())*6)/2,217,label,1,2,true,62);}
}
unsigned Renderer::present(DisplayPort& display,bool force){
    std::array<bool,240> changed{};unsigned count=0;
    for(unsigned row=0;row<240;++row){auto* current=canvas.pixels.data()+row*50;auto* old=previous_.data()+row*50;
        if(force||!valid_||std::memcmp(current,old,50)!=0){display.row(row,current);std::memcpy(old,current,50);changed[row]=true;++count;}}
    for(unsigned first=0;first<240;){if(!changed[first]){++first;continue;}unsigned last=first+1;while(last<240&&changed[last])++last;display.refresh(first,last-first);first=last;}
    valid_=true;return count;
}
}
