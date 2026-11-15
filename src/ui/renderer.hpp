// Original Sheets47 code, MIT license.
#pragma once
#include "app/application.hpp"
#include <array>
#include <cstdint>

namespace sheets47 {
class DisplayPort {
public:
    virtual ~DisplayPort() = default;
    virtual void row(unsigned y,const uint8_t* pixels)=0;
    virtual void refresh(unsigned first,unsigned count)=0;
};
class Canvas {
public:
    std::array<uint8_t,400*240/8> pixels{};
    void clear(){pixels.fill(0);}
    void pixel(int,int,bool=true);
    void rect(int,int,int,int,bool=true);
    void outline(int,int,int,int,int=1);
    void text(int,int,const std::string&,int=1,int=2,bool=false,int=400);
};
class Renderer {
public:
    Canvas canvas;
    void draw(const Application&);
    unsigned present(DisplayPort&,bool force=false);
    void invalidate(){valid_=false;}
private:
    std::array<uint8_t,400*240/8> previous_{};
    bool valid_=false;
    void worksheet(const Application&);
};
}
