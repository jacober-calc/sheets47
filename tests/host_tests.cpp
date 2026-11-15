// Original Sheets47 code, MIT license. Real Spreader + commands/storage/renderer.
#include "app/application.hpp"
#include "ui/renderer.hpp"
#include "platform/host/files.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <new>
#include <sstream>
#include <execinfo.h>
#include <exception>

// Deterministic failure injection for C++ allocations (not libc malloc).
static long fail_after=-1;
static long c_fail_after=-1;
static size_t live_bytes=0,peak_bytes=0,allocation_calls=0;
struct alignas(std::max_align_t) Allocation {size_t size;};
extern "C" void* __real_malloc(size_t);
extern "C" void* __real_realloc(void*,size_t);
extern "C" void* __wrap_malloc(size_t n){if(c_fail_after==0){errno=ENOMEM;return nullptr;}if(c_fail_after>0)--c_fail_after;return __real_malloc(n);}
extern "C" void* __wrap_realloc(void* p,size_t n){if(c_fail_after==0){errno=ENOMEM;return nullptr;}if(c_fail_after>0)--c_fail_after;return __real_realloc(p,n);}
void* operator new(std::size_t n){
    if(fail_after==0)throw std::bad_alloc();
    if(fail_after>0)--fail_after;
    auto* p=static_cast<Allocation*>(std::malloc(sizeof(Allocation)+std::max(n,size_t(1))));
    if(!p)throw std::bad_alloc();
    p->size=n;live_bytes+=n;peak_bytes=std::max(peak_bytes,live_bytes);++allocation_calls;return p+1;
}
void operator delete(void* p) noexcept {if(p){auto* h=static_cast<Allocation*>(p)-1;live_bytes-=h->size;std::free(h);}}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,std::size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,std::size_t) noexcept {::operator delete(p);}

using namespace sheets47;
static unsigned checks=0;
static std::string output(const char* name){auto* folder=std::getenv("SHEETS47_TEST_OUTPUT");return std::string(folder?folder:"build/host")+"/"+name;}
static std::string compact(std::string text){text.erase(std::remove(text.begin(),text.end(),' '),text.end());return text;}
static void require(bool ok,const char* message){++checks;if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
struct MemoryFiles final:FilePort {
    std::map<std::string,std::vector<uint8_t>> data;
    long fail_write_after=-1;
    bool partial=false,unavailable=false;
    unsigned writes=0;
    bool prepare() override{return !unavailable;}
    ReadResult read(const std::string& name,std::vector<uint8_t>& out,size_t max) override{
        auto it=data.find(name);if(it==data.end())return ReadResult::Missing;
        if(it->second.size()>max)return ReadResult::Error;
        out=it->second;return ReadResult::Ok;
    }
    bool write(const std::string& name,const std::vector<uint8_t>& in,bool exclusive) override{
        ++writes;if(exclusive&&data.count(name))return false;
        if(fail_write_after==0){if(partial)data[name]=std::vector<uint8_t>(in.begin(),in.begin()+in.size()/2);return false;}
        if(fail_write_after>0)--fail_write_after;
        data[name]=in;return true;
    }
};
struct RecordingDisplay final:DisplayPort {
    unsigned rows=0,flushes=0;
    void row(unsigned,const uint8_t*) override{++rows;}
    void refresh(unsigned,unsigned) override{++flushes;}
};
static void picture(Renderer& renderer,const std::string& path){
    std::ofstream out(path,std::ios::binary);out<<"P4\n400 240\n";
    out.write(reinterpret_cast<const char*>(renderer.canvas.pixels.data()),renderer.canvas.pixels.size());
    require(bool(out),"write host screenshot");
}
static void formulas(){
    Workbook w;require(w.demo(),"load demo");require(w.display({5,3})=="18.50","demo SUM");
    require(w.set({1,1},"5"),"edit quantity");require(w.display({1,3})=="12.50","dependent multiplication");
    require(w.display({5,3})=="23.50","dependent sum");
    require(w.set({8,0},"=IF(B2>3,ROUND(9.125,2),0)"),"IF/ROUND");
    require(w.display({8,0})=="9.13","nested formula result");
    const std::pair<const char*,const char*> functions[]={{"=AVERAGE(1,2,3)","2"},{"=MIN(1,2)","1"},{"=MAX(1,2)","2"},{"=COUNT(1,2)","2"},{"=ROUNDUP(1.21,1)","1.3"},{"=ROUNDDOWN(1.29,1)","1.2"}};
    for(auto [formula,expected]:functions){require(w.set({8,0},formula),"offered function accepted");require(w.display({8,0})==expected,"offered function calculates");}
    require(!w.set({8,0},"=SQRT(81)")&&!w.set({8,0},"=ABS(-9)"),"unimplemented upstream functions are not offered");
    require(w.set({9,0},"=1/0"),"error-valued formula retained");require(w.display({9,0})=="#DIV/0!","division error visible");
    require(w.set({9,1},"=SUM("),"invalid syntax remains editable");require(w.display({9,1})[0]=='#',"syntax error visible");
    require(!w.set({9,1},"=NOW()"),"unsupported volatile function rejected");
    require(!w.set({9,1},"=SUM(A:A)"),"unbounded column range rejected");
    require(!w.set({9,1},"=SUM(1:2)"),"unbounded row range rejected");
    require(!w.set({9,1},"=A257"),"reference bound enforced");
    require(!w.set({9,1},std::string(144,'A')),"input length enforced");
    require(w.set({10,0},"'00123"),"text literal");require(w.display({10,0})=="00123","numeric-looking text preserved");
    require(w.set({10,1},"=C11")&&w.set({10,2},"=B11"),"create circular dependency");
    require(w.display({10,1})[0]=='#',"cycle returns an error");
    std::puts("PASS formulas, dependency updates, cycles, errors, input limits");
}
static void edits(){
    Workbook w;require(w.demo(),"edit fixture");require(w.copy({{1,3},{1,3}},{2,3}),"copy formula");
    require(compact(w.input({2,3}))=="=B3*C3","relative copy references");
    require(w.set({1,3},"=B2*$C$2"),"absolute reference");require(w.copy({{1,3},{1,3}},{2,3}),"copy mixed formula");
    require(compact(w.input({2,3}))=="=B3*$C$2","absolute reference preserved");
    require(w.insert_row(1),"insert row");require(compact(w.input({2,3}))=="=B3*$C$3","insert rewrites references");
    require(w.erase_row(1),"delete inserted row");require(compact(w.input({1,3}))=="=B2*$C$2","delete reverses reference shift");
    Workbook refs;require(refs.set({0,0},"5")&&refs.set({1,1},"=A1"),"reference fixture");
    require(refs.erase_row(0),"delete referenced row");require(refs.display({0,1})=="#REF!","deleted reference error");
    require(refs.set({0,2},"7"),"edit after generated REF error");
    require(refs.input({0,1}).find("#REF!")!=std::string::npos,"REF error preserved across rebuild");
    Workbook copy;require(copy.demo(),"clipboard fixture");auto captured=copy.records();
    require(copy.set({1,1},"100"),"change source after copy");
    require(copy.paste(captured,{{1,1},{3,3}},{10,1}),"paste captured block");
    require(copy.display({10,3})=="7.50","clipboard uses captured inputs and shifted formulas");
    require(!copy.copy({{1,1},{3,3}},{255,25}),"reject out-of-bounds paste");
    Workbook filled;require(filled.demo(),"fill fixture");require(filled.fill_down({{1,3},{3,3}}),"fill down");
    require(compact(filled.input({3,3}))=="=B4*C4","fill down references");
    std::puts("PASS copy/paste snapshots, absolute/relative references, row edits, fill");
}
static void capacity(){
    std::vector<Record> data;
    for(unsigned i=0;i<MaxCells;++i)data.push_back({{uint16_t(i/16),uint8_t(i%16)},Format::General,std::to_string(i+1)});
    Workbook dense;require(dense.replace(data),"256 clustered cells");
    require(dense.display({15,15})=="256","clustered cell result");auto generation=dense.generation();
    require(!dense.set({255,25},"extra")&&dense.generation()==generation,"257th populated cell rejected atomically");
    for(unsigned i=0;i<MaxCells;++i)data[i].cell={uint16_t(i),uint8_t(i%26)};
    Workbook sparse;require(sparse.replace(data),"256 scattered cells");
    require(sparse.display({255,uint8_t(255%26)})=="256","scattered cell result");
    data.clear();
    for(unsigned i=0;i<128;++i)data.push_back({{uint16_t(i),0},Format::General,i?"=A"+std::to_string(i)+"+1":"1"});
    Workbook chain;require(chain.replace(data),"128-cell forward dependency chain");require(chain.display({127,0})=="128","forward chain result");
    for(unsigned i=0;i<128;++i)data[i].input=i==127?"1":"=A"+std::to_string(i+2)+"+1";
    require(chain.replace(data),"128-cell reverse dependency chain");require(chain.display({0,0})=="128","reverse chain result");
    Workbook scalar;require(!scalar.set({0,0},"=B1:B3"),"array/spill expression rejected");
    require(scalar.set({0,0},"=81^0.5")&&scalar.display({0,0})=="9","power expression");
    require(scalar.set({1,0},"=A256"),"last-row reference fixture");auto before=scalar.generation();
    require(!scalar.insert_row(0)&&scalar.generation()==before,"reference shifted beyond application bounds rejected");
    std::puts("PASS populated-cell cap, clustered/scattered cells, forward/reverse chains, array and structural bounds");
}
static unsigned allocation_failures(){
    unsigned failures=0;bool succeeded=false;
    for(long n=0;n<2000;++n){
        Workbook w;require(w.demo(),"allocation fixture");
        auto before=w.generation();std::string formula="=SUM(B2:B4)";
        fail_after=n;bool ok=w.set({8,1},formula);fail_after=-1;
        if(ok){succeeded=true;break;}
        ++failures;require(w.generation()==before,"allocation failure does not commit");
        require(w.display({5,3})=="18.50","allocation failure preserves calculations");
        require(w.input({8,1}).empty(),"failed cell edit absent");
        require(w.set({8,2},"8"),"worksheet usable after failed edit");
    }
    require(succeeded&&failures>5,"allocation failure sweep reached success");
    std::printf("PASS allocation failure at %u successive C++ allocation points\n",failures);return failures;
}
static unsigned c_allocation_failures(){
    unsigned failures=0;bool succeeded=false;
    for(long n=0;n<2000;++n){
        Workbook w;require(w.demo(),"C allocation fixture");std::string formula="=SUM(B2:B4)";auto before=w.generation();
        c_fail_after=n;bool ok=w.set({8,1},formula);c_fail_after=-1;
        if(ok){succeeded=true;break;}
        ++failures;require(w.generation()==before,"C allocation failure does not commit");
        require(w.display({5,3})=="18.50","C allocation failure preserves values");
        require(w.set({8,2},"8"),"workbook usable after C allocation failure");
    }
    require(succeeded&&failures>5,"C allocation failure sweep reached success");
    std::printf("PASS allocation failure at %u successive project malloc/realloc points\n",failures);return failures;
}
static void storage(){
    MemoryFiles files;Storage store(files);Workbook w;ViewState view;require(w.demo(),"storage fixture");
    view.active={1,3};require(store.save(w,view),"initial save");
    auto first=files.data;require(w.set({1,1},"7"),"save edit");require(store.save(w,view),"second generation");
    require(files.data.count("B0000001.S47")&&files.data.count("B0000002.S47"),"old generations retained");
    Workbook restored;ViewState restored_view;Storage reader(files);
    require(reader.load(restored,restored_view)==LoadResult::Loaded,"load saved workbook");
    require(restored.display({1,3})=="17.50"&&restored_view.active==view.active,"formulas, formats and viewport restored");
    files.data["B0000002.S47"][25]^=1;
    require(reader.load(restored,restored_view)==LoadResult::Loaded,"corrupt latest generation falls back");
    require(restored.display({1,3})=="7.50","fallback uses older valid workbook");
    files.fail_write_after=1;files.partial=true;
    require(!reader.save(w,view),"failed save after fallback reports failure");files.fail_write_after=-1;
    require(reader.load(restored,restored_view)==LoadResult::Loaded&&restored.display({1,3})=="7.50","failed save preserves the actually valid fallback head");
    for(long point=0;point<2;++point){
        files.data=first;files.fail_write_after=point;files.partial=true;Storage broken(files);
        require(!broken.save(w,view),"partial write fails Save");files.fail_write_after=-1;
        Storage recover(files);require(recover.load(restored,restored_view)==LoadResult::Loaded,"old save survives failed write");
        require(restored.display({1,3})=="7.50","failed save leaves prior generation");
    }
    MemoryFiles unavailable;unavailable.unavailable=true;Storage bad(unavailable);auto before=w.generation();
    require(bad.load(w,view)==LoadResult::Failed&&w.generation()==before,"failed load leaves workbook unchanged");
    require(!bad.save(w,view),"unavailable/full storage reports failure");
    std::puts("PASS save/load, CRC fallback, partial data/header writes, unavailable storage");
}
static void application(){
    MemoryFiles files;Storage store(files);Application app(store);Renderer renderer;RecordingDisplay display;
    require(app.start(),"empty startup");renderer.draw(app);require(renderer.present(display)==240,"initial full display");
    renderer.draw(app);require(renderer.present(display)==0,"unchanged frame sends no rows");
    require(app.handle({Key::Down}),"navigate");renderer.draw(app);auto changed=renderer.present(display);
    require(changed>0&&changed<240,"navigation changes only some display rows");
    app.handle({Key::F5});app.handle({Key::F4});require(app.workbook.display({5,3})=="18.50","demo through file menu");
    renderer.draw(app);picture(renderer,output("worksheet.pbm"));
    app.view.active={1,1};app.handle({Key::F1});app.handle({Key::Backspace});app.handle({Key::Digit,'5'});app.handle({Key::Enter});
    require(app.workbook.display({5,3})=="23.50","editor commit recalculates demo");
    app.handle({Key::F1});app.handle({Key::Backspace});app.handle({Key::Digit,'9'});app.handle({Key::Exit});
    require(app.workbook.input({1,1})=="5","cancel preserves stored content");
    app.view.active={1,3};app.handle({Key::F3});app.handle({Key::F2});app.handle({Key::Left});app.handle({Key::Left});app.handle({Key::Enter});
    app.handle({Key::Multiply});app.handle({Key::F2});app.handle({Key::Right});app.handle({Key::Enter});app.handle({Key::Enter});
    require(app.workbook.input({1,3})=="=B2*C2","reference picker edits original cell");
    app.handle({Key::F2});app.handle({Key::Down});renderer.draw(app);picture(renderer,output("range.pbm"));app.handle({Key::F6});
    require(app.save(),"application save");unsigned writes=files.writes;
    app.handle({Key::Down});require(app.suspend()&&files.writes==writes,"cursor-only sleep causes no storage writes");
    app.resumed();renderer.invalidate();renderer.draw(app);require(renderer.present(display)==240,"wake forces a full restore");
    app.handle({Key::F1});app.handle({Key::F1});require(app.mode==Mode::Alphabet,"on-screen letters reachable");app.handle({Key::F1});app.handle({Key::Enter});
    require(app.editor.back()=='A',"palette inserts letter and returns to editor");
    renderer.draw(app);picture(renderer,output("editor.pbm"));app.handle({Key::Exit});
    require(!app.dirty(),"draft cancellation leaves saved workbook clean");
    std::puts("PASS UI key flows, editing/ref picker, alphabet, partial redraw, suspend/resume model");
}
static Event named_key(const std::string& name){
    if(name.size()==2&&name[0]=='F'&&name[1]>='1'&&name[1]<='6')return {Key(int(Key::F1)+name[1]-'1')};
    if(name.size()==1&&name[0]>='0'&&name[0]<='9')return {Key::Digit,name[0]};
    static const std::pair<const char*,Key> names[]={{"UP",Key::Up},{"DOWN",Key::Down},{"LEFT",Key::Left},{"RIGHT",Key::Right},{"ENTER",Key::Enter},{"BSP",Key::Backspace},{"EXIT",Key::Exit},{"SHIFT",Key::Shift},{"+",Key::Add},{"-",Key::Subtract},{"*",Key::Multiply},{"/",Key::Divide},{".",Key::Dot}};
    for(auto [text,key]:names)if(name==text)return {key};
    return {};
}
static int repl(const std::string& root){
    HostFiles files(root);Storage storage(files);Application app(storage);Renderer renderer;app.start();
    std::string command;std::cout<<"READY\n"<<std::flush;
    while(std::getline(std::cin,command)){
        if(command=="quit")break;
        if(command.starts_with("key "))app.handle(named_key(command.substr(4)));
        else if(command.starts_with("text "))for(char c:command.substr(5))app.handle({Key::Character,c});
        else if(command.starts_with("frame ")){renderer.draw(app);picture(renderer,command.substr(6));}
        else if(command.starts_with("value ")){Cell c;if(parse_address(command.substr(6),c))std::cout<<app.workbook.display(c)<<'\n';}
        else if(command=="save")app.save();
        std::cout<<"OK\n"<<std::flush;
    }
    return 0;
}
int main(int argc,char** argv){
    std::set_terminate([](){fail_after=-1;c_fail_after=-1;void* frames[64];int n=backtrace(frames,64);backtrace_symbols_fd(frames,n,2);std::_Exit(2);});
    if(argc>=2&&std::string(argv[1])=="--repl")return repl(argc>=3?argv[2]:"build/host/SHEETS47");
    auto start=std::chrono::steady_clock::now();formulas();edits();capacity();auto failures=allocation_failures();auto c_failures=c_allocation_failures();storage();application();
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::ofstream report(output("validation.json"));
    report<<"{\n  \"checks\": "<<checks<<",\n  \"allocation_failure_points\": "<<failures<<",\n  \"c_allocation_failure_points\": "<<c_failures<<",\n  \"host_test_ms\": "<<ms<<",\n  \"host_cpp_peak_live_bytes\": "<<peak_bytes<<",\n  \"note\": \"Host only; peak-byte instrumentation excludes direct libc allocations and target heap/stack. C fault injection wraps project malloc/realloc calls, not hidden calls inside precompiled shared libraries.\"\n}\n";
    std::printf("PASS %u checks in %.2f ms; host C++ peak live bytes %zu (not target measurements)\n",checks,ms,peak_bytes);
}
