#include "stdafx.h"
#include <fstream>
#include <cassert>
#include "ColorPalette.h"
#include "DibUtils.h"

ColorPalette::ColorPalette(void)
{
    m_flag = true;
}

ColorPalette::~ColorPalette(void)
{
}

bool ColorPalette::init(Configuration& config)
{
    std::string palette_path = config.get_path("palette");

    //
    // loading palettes
    //
    std::ifstream palette_file(palette_path, std::ios_base::in | std::ios_base::binary);

    if (!palette_file.is_open())
    {
        assert(false && _T("Couldn't open source file PAL."));
        return false;
    }

    for (int i = 0; i < 256; i++) {
        m_pal[i].rgbRed = palette_file.get();
        m_pal[i].rgbGreen = palette_file.get();
        m_pal[i].rgbBlue = palette_file.get();
        m_pal[i].rgbReserved = 0; // opaque

        if (m_pal[i].rgbRed   > 0) { m_pal[i].rgbRed <<= 2; m_pal[i].rgbRed |= 0x03; }
        if (m_pal[i].rgbGreen > 0) { m_pal[i].rgbGreen <<= 2; m_pal[i].rgbGreen |= 0x03; }
        if (m_pal[i].rgbBlue  > 0) { m_pal[i].rgbBlue <<= 2; m_pal[i].rgbBlue |= 0x03; }
    }

    palette_file.close();

    return true;
}

void ColorPalette::draw(DibSection& ds, int ox, int oy)
{
    for (int i = 0; i < 256; i++)
    {
        int x = ox + (i % 16) * 8;
        int y = oy + (i / 16) * 8;
        FillRect(ds.bits(y) + x, 8, 8, i, 1, ds.ypitch());
    }
}

void ColorPalette::update(DibSection& ds)
{
    rotate_palette(0xe0, 8); // fires, braziers, candles
    rotate_palette(0xe8, 8); // BluGlo[tm] magical items

    if (m_flag)
    {
        rotate_palette(0xf0, 4); // purple braziers, force fields
        rotate_palette(0xf4, 4); // kitchen cauldrons
        rotate_palette(0xf8, 4); // poison fields
    }
    m_flag = !m_flag;

    ds.apply_palette(256, m_pal);
}

void ColorPalette::rotate_palette(int start, int count)
{
    RGBQUAD first = m_pal[start];
    count--;
    for (int i = 0; i < count; i++)
    {
        m_pal[start + i] = m_pal[start + i + 1];
    }
    m_pal[start + count] = first;
}
