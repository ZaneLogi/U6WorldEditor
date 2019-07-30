#include "stdafx.h"
#include <cassert>
#include "DibSection.h"

DibSection::DibSection() : m_bitmap(nullptr)
{
}


DibSection::~DibSection()
{
    clear();
}

void DibSection::clear()
{
    if (m_bitmap) {
        ::DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }
}

bool DibSection::create(int w, int h, int bitcount, int palsize, const RGBQUAD* pal)
{
    clear();

    switch (bitcount) {
    case 1:
        m_palette_size = 2;
        m_xpitch = 1;
        break;
    case 4:
        m_palette_size = 16;
        m_xpitch = 1;
        break;
    case 8:
        m_palette_size = 256;
        m_xpitch = 1;
        break;
    case 16:
        if (palsize == 3)   m_palette_size = 3;
        else                m_palette_size = 0;
        m_xpitch = 2;
        break;
    case 24:
        m_palette_size = 0;
        m_xpitch = 3;
        break;
    case 32:
        if (palsize == 3)   m_palette_size = 3;
        else                m_palette_size = 0;
        m_xpitch = 4;
        break;
    default:
        assert(_T("Not 1, 4, 8, 16, 24, 32 bits.\n"));
        return false;
    }

    m_bitmap_info_alloc_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * m_palette_size;
    m_bits_per_pixel = bitcount;
    m_bits_per_line = w * m_bits_per_pixel;
    m_bytes_per_line = (m_bits_per_line + 7) >> 3;
    m_pitch_per_line = (m_bytes_per_line + 3) & (~3);
    m_image_bytes = h * m_pitch_per_line;

    m_ypitch = -m_pitch_per_line;

    // Allocate Memory
    m_bitmap_info_buffer.resize(m_bitmap_info_alloc_size);
    m_bitmap_info = (PBITMAPINFO)m_bitmap_info_buffer.data();

    m_bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bitmap_info->bmiHeader.biWidth = w;
    m_bitmap_info->bmiHeader.biHeight = h;
    m_bitmap_info->bmiHeader.biPlanes = 1;
    m_bitmap_info->bmiHeader.biBitCount = bitcount;
    m_bitmap_info->bmiHeader.biCompression = (m_palette_size != 3) ? BI_RGB : BI_BITFIELDS;
    m_bitmap_info->bmiHeader.biSizeImage = 0;
    m_bitmap_info->bmiHeader.biXPelsPerMeter = 0;
    m_bitmap_info->bmiHeader.biYPelsPerMeter = 0;
    m_bitmap_info->bmiHeader.biClrUsed = 0;
    m_bitmap_info->bmiHeader.biClrImportant = 0;

    if (m_bitmap_info->bmiHeader.biCompression == BI_RGB) {
        if (m_palette_size) {
            RGBQUAD* prgb = m_bitmap_info->bmiColors;
            int copysize;
            if (0 >= palsize || !pal) {
                copysize = 0;
            }
            else {
                copysize = min(palsize, m_palette_size);
                memcpy(prgb, pal, sizeof(RGBQUAD) * copysize);
            }

            for (int c = copysize; c < m_palette_size; c++) {
                prgb[c].rgbRed = prgb[c].rgbGreen = prgb[c].rgbBlue = 255 * c / (m_palette_size - 1);
                prgb[c].rgbReserved = 0;
            }
        }
    }
    else { // BI_BITFIELDS
        LPDWORD pmask = (LPDWORD)m_bitmap_info->bmiColors;
        if (pal) {
            memcpy(pmask, pal, sizeof(DWORD) * 3);
        }
        else {
            if (bitcount == 16) { // 5-5-5
                *pmask++ = 0x7C00; // red
                *pmask++ = 0x03E0; // green
                *pmask = 0x001F; // blue
            }
            else { // bitcount = 32
                *pmask++ = 0x00FF0000; // red
                *pmask++ = 0x0000FF00; // green
                *pmask = 0x000000FF; // blue
            }
        }
    }

    HDC hdc = GetDC(NULL);
    m_bitmap = CreateDIBSection(
        hdc,
        m_bitmap_info,
        DIB_RGB_COLORS,
        (PVOID*)&m_bits, NULL, 0);
    ReleaseDC(NULL, hdc);

    if (m_bitmap && m_bits) {
        m_offsets.resize(h);
        for (int i = 0; i < h; i++) {
            m_offsets[i] = m_bits + (h - i - 1) * m_pitch_per_line;
        }

        memset(m_bits, 0, m_image_bytes);

        assert((DWORD_PTR(m_bits) & 0xf) == 0); // make sure we are at least 128 bit aligned.(SSE2)
    }


    return (m_bitmap != NULL);
}

bool DibSection::create_gray_scale(int w, int h)
{
    RGBQUAD rgb[256];
    for (int i = 0; i < 256; i++) {
        rgb[i].rgbRed = rgb[i].rgbGreen = rgb[i].rgbBlue = (BYTE)i;
        rgb[i].rgbReserved = 0;
    }
    return create(w, h, 8, 256, rgb);
}

void DibSection::apply_palette(int palette_size, const RGBQUAD* rgbquads)
{
    memcpy(m_bitmap_info->bmiColors, rgbquads, palette_size * sizeof(RGBQUAD));
}
