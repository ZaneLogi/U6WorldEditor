#pragma once
#include "Configuration.h"
#include "DibSection.h"

class Text
{
public:
    bool init(Configuration& config);
    void draw_char(DibSection& ds, uint8_t ch, uint16_t x, uint16_t y, uint8_t color);

private:
    bool load_u6_fonts(Configuration& config);
    bool load_wou_fonts(Configuration& config);

private:
    std::vector<uint8_t> m_font;

};