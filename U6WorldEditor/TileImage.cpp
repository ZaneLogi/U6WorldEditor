#include "stdafx.h"
#include "TileImage.h"
#include "LZW.h"

const int MAX_TILE_COUNT = 2048;

inline uint32_t FILESIZE(std::ifstream& f)
{
    auto cur_pos = f.tellg();
    f.seekg(0, std::ios::end);
    auto fsize = f.tellg();
    f.seekg(cur_pos, std::ios::beg);
    return (uint32_t)fsize;
}

bool TileImage::init(Configuration& config)
{
    load_masktype(config);
    load_tileindex(config);
    load_maptiles(config);
    load_objtiles(config);
    load_animmask(config);

    // debug code
#if 0
    uint32_t dw;
    for (dw = 0; dw < 512; dw++) {
        int tile_data_offset = m_tileindex[dw] << 4;
        auto p = m_maptiles.data() + tile_data_offset;
        TRACE("%d: %d, mask: %d\n", dw, tile_data_offset, m_masktype[dw]);
    }

    for (; dw < 2048; dw++) {
        auto tile_data_offset = (m_tileindex[dw] << 4) - m_maptiles.size();
        auto p = m_objtiles.data() + tile_data_offset;
        TRACE("%d: %d, mask: %d\n", dw, tile_data_offset, m_masktype[dw]);
    }
#endif

    return true;
}

void TileImage::draw(DibSection& ds, int x, int y, uint16_t tile_index)
{
    assert(tile_index < MAX_TILE_COUNT && "tile_index must be lower than 2048");

    auto dst = ds.bits(y) + x;
    auto ypitch = ds.ypitch();

    uint8_t* src = nullptr;
    if (tile_index < 512)
    {
        int tile_data_offset = (m_tileindex[tile_index] << 4);
        src = m_maptiles.data() + tile_data_offset;
    }
    else {
        int tile_data_offset = (m_tileindex[tile_index] << 4) - (int)m_maptiles.size();
        src = m_objtiles.data() + tile_data_offset;
    }

    switch (m_masktype[tile_index]) {
    case 0x05: // U6TILE_TRANS
        draw_transparent_tile(dst, ypitch, src);
        break;
    case 0x00: // U6TILE_PLAIN
        draw_plain_tile(dst, ypitch, src);
        break;
    case 0x0a: // U6TILE_PBLCK
        draw_pixelblock_tile(dst, ypitch, src);
        break;
    }
}

bool TileImage::load_masktype(Configuration& config)
{
    auto masktype_filename = config.get_path("masktype_vga");
    uint32_t filesize = 0;
    uint32_t datasize = 0;

    //
    // loading Mask Type
    //
    std::ifstream masktype_file(masktype_filename, std::ios_base::in | std::ios_base::binary);
    if (!masktype_file.is_open())
    {
        assert(false && "Couldn't open source file MASKTYPE.VGA!");
        return false;
    }

    filesize = FILESIZE(masktype_file);
    masktype_file.read((char*)&datasize, sizeof(datasize));

    if (filesize == datasize)
    { // no compression, SAVAGE
        struct { uint32_t offset : 24; uint32_t flag : 8; } data_offset;
        masktype_file.read((char*)&data_offset, sizeof(data_offset));
        assert(data_offset.offset == 8); // data_offset.flag = 0x02 (se), 0xe0 (md) => uncompressed data
        datasize = filesize - data_offset.offset;
        m_masktype.resize(datasize);
        masktype_file.read((char*)m_masktype.data(), datasize);
        assert((uint32_t)masktype_file.tellg() - 8 == datasize && "The data is incompelete!");
    }
    else
    { // U6
        std::stringstream decoded_stream;
        LZW().decode(masktype_file, decoded_stream, 8);
        m_masktype.resize(datasize);
        decoded_stream.read((char*)m_masktype.data(), datasize);
        uint32_t decoded_size = (uint32_t)decoded_stream.tellg();
        assert(decoded_size == datasize && "The decoded size is incorrect!");
    }

    masktype_file.close();
    return true;
}

bool TileImage::load_tileindex(Configuration& config)
{
    auto tileindex_filename = config.get_path("tileindex_vga");
    uint32_t filesize = 0;
    uint32_t datasize = 0;

    //
    // loading Tile Index
    //
    std::ifstream tileindex_file(tileindex_filename, std::ios_base::in | std::ios_base::binary);
    if (!tileindex_file.is_open())
    {
        assert(false && "Couldn't open source file TILEINDEX.VGA!");
        return false;
    }

    m_tileindex.resize(FILESIZE(tileindex_file));
    tileindex_file.read((char*)m_tileindex.data(), m_tileindex.size());

    tileindex_file.close();
    return true;
}

bool TileImage::load_maptiles(Configuration& config)
{
    auto maptiles_filename = config.get_path("maptiles_vga");
    uint32_t filesize = 0;
    uint32_t datasize = 0;

    //
    // loading MapTiles
    //
    std::ifstream maptiles_file(maptiles_filename, std::ios_base::in | std::ios_base::binary);
    if (!maptiles_file.is_open())
    {
        assert(false && "Couldn't open source file MAPTILES.VGA!");
        return false;
    }

    filesize = FILESIZE(maptiles_file);
    maptiles_file.read((char*)&datasize, sizeof(datasize));

    if (filesize == datasize)
    { // no compression
        struct { uint32_t offset : 24; uint32_t flag : 8; } data_offset;
        maptiles_file.read((char*)&data_offset, sizeof(data_offset));
        assert(data_offset.offset == 8); // data_offset.flag = 0x02 (se), 0xe0 (md) => uncompressed data
        datasize = filesize - data_offset.offset;
        m_maptiles.resize(datasize);
        maptiles_file.read((char*)m_maptiles.data(), datasize);
        assert((uint32_t)maptiles_file.tellg() - 8 == datasize && "The data is incompelete!");
    }
    else
    {
        std::stringstream decoded_stream;
        LZW().decode(maptiles_file, decoded_stream, 8);
        m_maptiles.resize(datasize);
        decoded_stream.read((char*)m_maptiles.data(), datasize);
        uint32_t decoded_size = (uint32_t)decoded_stream.tellg();
        assert(decoded_size == datasize && "The decoded size is incorrect!");
    }

    maptiles_file.close();
    return true;
}

bool TileImage::load_objtiles(Configuration& config)
{
    auto objtiles_filename = config.get_path("objtiles_vga");
    uint32_t filesize = 0;
    uint32_t datasize = 0;

    //
    // loading ObjTiles
    //
    std::ifstream objtiles_file(objtiles_filename, std::ios_base::in | std::ios_base::binary);
    if (!objtiles_file.is_open())
    {
        assert(false && "Couldn't open source file OBJTILES.VGA!");
        return false;
    }

    filesize = FILESIZE(objtiles_file);
    m_objtiles.resize(filesize);
    objtiles_file.read((char*)m_objtiles.data(), filesize);
    objtiles_file.close();
    return true;
}

bool TileImage::load_animmask(Configuration& config)
{
    auto animmask_filename = config.get_path("animmask_vga");
    if (animmask_filename.empty())
        return true; // only U6 has this file.

    std::ifstream animmask_file(animmask_filename, std::ios_base::in | std::ios_base::binary);
    if (!animmask_file.is_open())
    {
        assert(false && "Couldn't open source file ANIMMASK.VGA!");
        return false;
    }

    uint32_t filesize = 0;
    uint32_t datasize = 0;

    filesize = FILESIZE(animmask_file);
    animmask_file.read((char*)&datasize, sizeof(datasize));

    std::stringstream decoded_stream;
    LZW().decode(animmask_file, decoded_stream, 8);

    uint32_t decoded_size = (uint32_t)decoded_stream.tellp();
    assert(decoded_size == datasize && "The decoded size is incorrect!");

    animmask_file.close();

    //
    // Make the 32 tiles from index 16 onwards transparent with data from animmask.vga
    //
    for (int i = 16; i < 48; i++)
    {
        m_masktype[i] = 0x05; // make it transparent

        int tile_data_offset = (m_tileindex[i] << 4);
        auto dst = m_maptiles.data() + tile_data_offset;

        decoded_stream.seekg((i - 16) * 64, std::ios::beg);

        int clen = decoded_stream.get();
        int displacement = 0;

        do
        {
            if (displacement > 0)
            {
                dst += displacement;
            }

            if (clen > 0)
            {
                memset(dst, 0xff, clen);
                dst += clen;
            }

            displacement = decoded_stream.get();
            clen = decoded_stream.get();

        } while (displacement != 0 && clen != 0);
    }

    return true;
}

void TileImage::draw_transparent_tile(uint8_t* dst, int ypitch, uint8_t* src)
{
    const int TILE_HEIGHT = 16;
    const int TILE_WIDTH = 16;
    for (int y = 0; y < TILE_HEIGHT; y++)
    {
        auto p = dst;
        for (int x = 0; x < TILE_WIDTH; x++)
        {
            auto c = *src++;
            if (c != 0xff)
                *p++ = c;
            else
                p++;
        }
        dst += ypitch;
    }
}

void TileImage::draw_plain_tile(uint8_t* dst, int ypitch, uint8_t* src)
{
    const int TILE_HEIGHT = 16;
    const int TILE_WIDTH = 16;
    for (int i = 0; i < TILE_HEIGHT; i++)
    {
        memcpy(dst, src, TILE_WIDTH);
        dst += ypitch;
        src += TILE_WIDTH;
    }
}

void TileImage::draw_pixelblock_tile(uint8_t* dst, int ypitch, uint8_t* src)
{
    uint8_t lines = *src++; // how many line to decode

    int skip_vertical_lbyte = (*src >> 4);
    int skip_vertical_hbyte = (*(src+1) & 0x0F);

    int skip_lines = ((skip_vertical_lbyte | (skip_vertical_hbyte << 4)) / 11);

    dst += ypitch * skip_lines;

    int x = 0;
    while (1)
    {
        int b0 = *src++;
        int b1 = *src++;
        int b2 = *src++;
        if (b0 == 0 && b1 == 0 && b2 == 0)
            break;

        int skip_pixels = b0 & 0x0f;
        x += skip_pixels;
        if (x >= 16)
        {
            dst += ypitch;
            x -= 16;
        }

        memcpy(dst + x, src, b2);
        src += b2;
        x += b2;
        if (x >= 16)
        {
            dst += ypitch;
            x -= 16;
        }
    }
}