#pragma once
#include <vector>

class DibSection
{
public:
    DibSection();
    virtual ~DibSection();
    virtual void clear();

    DibSection(const DibSection&) = delete;
    DibSection& operator = (const DibSection&) = delete;

    bool create(int w, int h, int bitcount, int palsize = 0, const RGBQUAD* pal = nullptr);
    bool create_gray_scale(int w, int h);
    void apply_palette(int palette_size, const RGBQUAD* rgbquads);

    HBITMAP get_handle() const { return m_bitmap; }
    HBITMAP operator()() const { return m_bitmap; }
    operator HBITMAP() const { return m_bitmap; }

    PBYTE bits() const { return m_bits; }
    PBYTE bits(int line_index) const { return m_offsets[line_index]; }
    int ypitch() const { return m_ypitch; }
    const BITMAPINFO* bitmap_info() const { return m_bitmap_info; }
    const uint32_t width() const { return m_bitmap_info->bmiHeader.biWidth; }
    const uint32_t height() const { return m_bitmap_info->bmiHeader.biHeight; }

protected:
    HBITMAP             m_bitmap;
    PBYTE               m_bits;
    PBITMAPINFO         m_bitmap_info;
    std::vector<char>   m_bitmap_info_buffer;
    std::vector<PBYTE>  m_offsets;

    int                 m_palette_size;
    int                 m_xpitch;
    int                 m_ypitch;
    int                 m_bitmap_info_alloc_size;
    int                 m_bits_per_pixel;
    int                 m_bits_per_line;
    int                 m_bytes_per_line;
    int                 m_pitch_per_line;
    int                 m_image_bytes;
};

