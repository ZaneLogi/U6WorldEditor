#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "Text.h"
#include "LZW.h"

#define FONT_COLOR_U6_NORMAL            0x48
#define FONT_COLOR_U6_HIGHLIGHT         0x0c
#define FONT_COLOR_WOU_NORMAL           0
#define FONT_COLOR_WOU_CONVERSE_INPUT   1

#define FONT_COLOR_WOU_HIGHLIGHT        4

#define FONT_UP_ARROW_CHAR              19
#define FONT_DOWN_ARROW_CHAR            20

inline uint32_t FILESIZE(std::ifstream& f)
{
    auto cur_pos = f.tellg();
    f.seekg(0, std::ios::end);
    auto fsize = f.tellg();
    f.seekg(cur_pos, std::ios::beg);
    return (uint32_t)fsize;
}

bool Text::init(Configuration& config)
{
    auto game_type = config.get_property("game_type");
    if (game_type == "u6")
    {
        load_u6_fonts(config);
    }
    else
    {
        load_wou_fonts(config);
    }
    return true;
}

void Text::draw_char(DibSection& ds, uint8_t ch, uint16_t x, uint16_t y, uint8_t color)
{
    // 256 characters
    auto p = m_font.data();
    uint16_t font_height = *(uint16_t*)p;
    uint8_t foreground_color = *(uint8_t*)(p + 2);
    uint8_t background_color = *(uint8_t*)(p + 3);

    auto char_width = p + 4 + ch;
    auto pixel_data_offset_low  = p + 0x104 + ch;
    auto pixel_data_offset_high = p + 0x204 + ch;

    auto pixel_data = p + (*pixel_data_offset_low | (*pixel_data_offset_high << 8));
    int width = *char_width;
    int height = font_height;

    auto dst = ds.bits(y) + x;
    auto pitch = ds.ypitch();
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (*pixel_data == foreground_color)
            {
                *(dst + j) = color;
            }
            pixel_data++;
        }
        dst += pitch;
    }
}

bool Text::load_u6_fonts(Configuration& config)
{
    auto font_filename = config.get_path("font");
    //
    // loading u6.ch
    //
    std::ifstream font_file(font_filename, std::ios_base::in | std::ios_base::binary);
    if (!font_file.is_open())
    {
        assert(false && "Couldn't open source file U6.CH.");
        return false;
    }

    auto filesize = FILESIZE(font_file);

    std::vector<uint8_t> font_data;
    font_data.resize(filesize);
    font_file.read((char*)font_data.data(), filesize);
    font_file.close();

    // convert the data to be same as the one for World of Ultima
    m_font.resize(4 + 256 * 3 + 64 * 256);
    auto p = m_font.data();
    *p = 8; // the height of the character
    *(p + 1) = 0;
    *(p + 2) = 1; // foreground color
    *(p + 3) = 0; // background color

    for (int i = 0; i < 256; i++)
    {
        *(p + 4 + i) = 8; // the width of the character
        uint16_t offset = 0x304 + 64 * i;
        *(p + 0x104 + i) = (offset & 0xff);
        *(p + 0x204 + i) = (offset >> 8);

        auto src = font_data.data() + 8 * i;
        auto dst = p + offset;
        for (int y = 0; y < 8; y++)
        {
            uint8_t c = *src++;
            for (int x = 0; x < 8; x++)
            {
                *dst = (c & 1) ? 1 : 0;
                c >>= 1;
                dst++;
            }
        }
    }

    return true;
}

bool Text::load_wou_fonts(Configuration& config)
{
    auto font_filename = config.get_path("font");

    std::ifstream font_file(font_filename, std::ios_base::in | std::ios_base::binary);
    if (!font_file.is_open())
    {
        assert(false && "Couldn't open source file SAVAGE.FNT!");
        return false;
    }

    uint32_t filesize = 0;
    uint32_t datasize = 0;

    filesize = FILESIZE(font_file);
    font_file.read((char*)&datasize, sizeof(datasize));
    assert(filesize == datasize);

    struct { uint32_t offset : 24; uint32_t flag : 8; } data_offset;
    font_file.read((char*)&data_offset, sizeof(data_offset)); // data_offset.flag = 0x01(se), 0x20(md) => compressed data

    font_file.seekg(data_offset.offset); // go to the data section

    uint32_t size;
    font_file.read((char*)&size, sizeof(size)); // read the size of the uncompressed data

    std::stringstream decoded_stream;
    LZW().decode(font_file, decoded_stream, 8); // decode the compressed data

    assert(size == decoded_stream.tellp());

    font_file.close();

    m_font.resize(size);
    decoded_stream.read((char*)m_font.data(), size); // copy the uncompressed data

    return true;
}
