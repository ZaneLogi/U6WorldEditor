#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include "LZW.h"
#include "TileManager.h"

inline uint32_t FILESIZE(std::ifstream& f)
{
    auto cur_pos = f.tellg();
    f.seekg(0, std::ios::end);
    auto fsize = f.tellg();
    f.seekg(cur_pos, std::ios::beg);
    return (uint32_t)fsize;
}

TileManager::TileManager()
{
}

TileManager::~TileManager()
{
}

bool TileManager::init(Configuration& config, TileImage* tile_image)
{
    auto game_type = config.get_property("game_type");
    if (game_type == "u6") { m_update_fn = &TileManager::update_u6; }
    else if (game_type == "se") { m_update_fn = &TileManager::update_se; }
    else if (game_type == "md") { m_update_fn = &TileManager::update_md; }

    m_tile_image = tile_image;

    load_animdata(config);
    load_basetile(config);
    load_tileflag(config);
    load_look(config);

    for (int i = 0; i < 2048; i++)
    {
        m_anim_tile_index[i] = i;
    }

    return true;
}

void TileManager::update()
{
    m_anim_counter++;
    for (int i = 0; i < m_animdata.number_of_tiles_to_animate; i++)
    {
        int current_anim_frame = ((m_anim_counter & m_animdata.and_masks[i]) >> m_animdata.shift_values[i]);
        m_anim_tile_index[m_animdata.tile_to_animate[i]] = m_animdata.first_anim_frame[i] + current_anim_frame;
    }

    (this->*m_update_fn)();
}

void TileManager::update_u6()
{
}

void TileManager::update_se()
{
}

void TileManager::update_md()
{
    // water in the canal
    int full_water_in_canal = 1; // 0: empty, 1: full
    for (int i = 0; i < 8; i++)
    {
        m_anim_tile_index[m_animdata.tile_to_animate[i]] = m_animdata.first_anim_frame[i] + full_water_in_canal;
    }

    for (int i = 27; i < 31; i++)
    {
        m_anim_tile_index[m_animdata.tile_to_animate[i]] = m_animdata.first_anim_frame[i] + full_water_in_canal;
    }
}

void TileManager::draw(DibSection& ds, int x, int y, uint16_t tile_index)
{
    m_tile_image->draw(ds, x, y, m_anim_tile_index[tile_index]);
}

void TileManager::draw(DibSection& ds, int x, int y, uint16_t obj_number, uint8_t obj_frame, bool toptile)
{
    int tile_index = m_obj_to_tile[obj_number] + obj_frame;
    auto flags = m_tile_flags2[tile_index];
    bool is_top = (flags & 0x10) != 0;
    bool is_double_width = (flags & 0x80) != 0;
    bool is_double_height = (flags & 0x40) != 0;

    if (is_top == toptile)
    {
        m_tile_image->draw(ds, x, y, m_anim_tile_index[tile_index]);
    }

    int x1 = x - 16;
    int y1 = y - 16;

    if (is_double_width && x1 >= 0)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top == toptile)
        {
            m_tile_image->draw(ds, x1, y, m_anim_tile_index[tile_index]);
        }
    }

    if (is_double_height && y1 >= 0)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top == toptile)
        {
            m_tile_image->draw(ds, x, y1, m_anim_tile_index[tile_index]);
        }
    }

    if (is_double_width && is_double_height && x1 >= 0 && y1 >= 0)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top == toptile)
        {
            m_tile_image->draw(ds, x1, y1, m_anim_tile_index[tile_index]);
        }
    }
}

std::string TileManager::get_description(uint16_t tile_index)
{
    assert(tile_index < 2048 && "out of range, should be less than 2048!");
    return std::string((char*)m_tile_look[tile_index]);
}

std::string TileManager::get_description(uint16_t obj_number, uint8_t obj_frame, uint8_t quantity)
{
    int tile_index = m_obj_to_tile[obj_number] + obj_frame;
    std::string name((char*)m_tile_look[tile_index]);

    auto singular_pos = name.find('/');
    auto plural_pos = name.find('\\');
    if (quantity > 1 || plural_pos != std::string::npos)
    {
        std::string singular_form;
        std::string plural_form;
        std::string prefix = name;
        if (plural_pos != std::string::npos)
        {
            plural_form = name.substr(plural_pos + 1);
            prefix = name.substr(0, plural_pos);
        }
        if (singular_pos != std::string::npos)
        {
            singular_form = name.substr(singular_pos + 1, plural_pos - singular_pos - 1);
            prefix = name.substr(0, singular_pos);
        }

        char tmp[32];
        sprintf_s(tmp, "%d %s%s",
            quantity,
            prefix.c_str(),
            quantity > 1 ? plural_form.c_str() : singular_form.c_str());
        return std::string(tmp);
    }
    else
    {
        return name;
    }
}

TileInfo TileManager::get_info(uint16_t obj_number, uint8_t obj_frame)
{
    TileInfo ti;
    int tile_index = m_obj_to_tile[obj_number] + obj_frame;
    ti.index = tile_index;
    ti.flag1 = m_tile_flags1[tile_index];
    ti.flag2 = m_tile_flags2[tile_index];
    ti.flag3 = m_tile_flags3[tile_index];
    return ti;
}

bool TileManager::is_big_flat_object(uint16_t obj_number, uint8_t obj_frame, bool& double_width, bool& double_height)
{
    int tile_index = m_obj_to_tile[obj_number] + obj_frame;
    auto flags = m_tile_flags2[tile_index];
    bool is_top = (flags & 0x10) != 0;
    double_width = (flags & 0x80) != 0;
    double_height = (flags & 0x40) != 0;

    if (is_top || (!double_width && !double_height))
        return false;

    if (double_width)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top)
            return false;
    }

    if (double_height)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top)
            return false;
    }

    if (double_width && double_height)
    {
        tile_index--;
        flags = m_tile_flags2[tile_index];
        is_top = (flags & 0x10) != 0;
        if (is_top)
            return false;
    }

    return true;
}

bool TileManager::load_animdata(Configuration& config)
{
    auto animdata_filename = config.get_path("animdata");

    std::ifstream animdata_file(animdata_filename, std::ios_base::in | std::ios_base::binary);
    if (!animdata_file.is_open())
    {
        assert(false && "Couldn't open source file ANIMDATA!");
        return false;
    }

    animdata_file.read((char*)&m_animdata, sizeof(m_animdata));
    animdata_file.close();
    return true;
}

bool TileManager::load_basetile(Configuration& config)
{
    auto basetile_filename = config.get_path("basetile");

    std::ifstream basetile_file(basetile_filename, std::ios_base::in | std::ios_base::binary);
    if (!basetile_file.is_open())
    {
        assert(false && "Couldn't open source file BASETILE!");
        return false;
    }

    basetile_file.read((char*)m_obj_to_tile, sizeof(m_obj_to_tile));
    basetile_file.close();

    return true;
}

bool TileManager::load_tileflag(Configuration& config)
{
    auto tileflag_filename = config.get_path("tileflag");

    //
    // loading Tile Flag
    //
    std::ifstream tileflag_file(tileflag_filename, std::ios_base::in | std::ios_base::binary);
    if (!tileflag_file.is_open())
    {
        assert(false && "Couldn't open source file TILEFLAG!");
        return false;
    }

    tileflag_file.read((char*)m_tile_flags1, sizeof(m_tile_flags1));
    tileflag_file.read((char*)m_tile_flags2, sizeof(m_tile_flags2));

    tileflag_file.seekg(0x1400);
    tileflag_file.read((char*)m_tile_flags3, sizeof(m_tile_flags3)); // '', 'a', 'an', 'the'

    tileflag_file.close();

    return true;
}

bool TileManager::load_look(Configuration& config)
{
    auto look_filename = config.get_path("look");
    uint32_t filesize = 0;
    uint32_t datasize = 0;

    std::ifstream look_file(look_filename, std::ios_base::in | std::ios_base::binary);
    if (!look_file.is_open())
    {
        assert(false && "Couldn't open source file LOOK!");
        return false;
    }

    filesize = FILESIZE(look_file);
    look_file.read((char*)&datasize, sizeof(datasize));

    if (filesize == datasize)
    { // no compression
        struct { uint32_t offset : 24; uint32_t flag : 8; } data_offset;
        look_file.read((char*)&data_offset, sizeof(data_offset));
        assert(data_offset.offset == 8); // data_offset.flag = 0x02 (se), 0xe0 (md) => uncompressed data
        datasize = filesize - data_offset.offset;
        m_look_data.resize(datasize);
        look_file.read((char*)m_look_data.data(), datasize);
        assert((uint32_t)look_file.tellg() - 8 == datasize && "The data is incompelete!");
    }
    else
    {
        std::stringstream decoded_stream;
        LZW().decode(look_file, decoded_stream, 8);
        m_look_data.resize(datasize);
        decoded_stream.read((char*)m_look_data.data(), datasize);
        uint32_t decoded_size = (uint32_t)decoded_stream.tellg();
        assert(decoded_size == datasize && "The decoded size is incorrect!");
    }

    look_file.close();

    // parsing...
    auto p = m_look_data.data();
    for (int i = 0; i < 2048; i++)
    {
        int tile_index = *p + *(p + 1) * 256;
        p += 2;
        for (int j = i; j <= tile_index && j < 2048; j++)
        {
            m_tile_look[j] = p;
        }

        i = tile_index;

        while (*p != 0)
        {
            p++;
        }
        p++; // skip '\0'
    }

    return true;
}
