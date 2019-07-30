#pragma once
#include "DibSection.h"
#include "Configuration.h"

class ColorPalette
{
public:
    ColorPalette(void);
    ~ColorPalette(void);

    bool init(Configuration& config);
    void draw(DibSection& ds, int ox, int oy);
    void update(DibSection& ds);

    const RGBQUAD* color_entries() const { return m_pal; }

private:
    void rotate_palette(int start, int count);

private:
    RGBQUAD m_pal[256];
    bool    m_flag;


};