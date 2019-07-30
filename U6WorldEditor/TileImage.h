#pragma once
#include "Configuration.h"
#include "DibSection.h"

class TileImage
{
public:
    bool init(Configuration& config);
    void draw(DibSection& ds, int x, int y, uint16_t tile_index);

private:
    bool load_masktype(Configuration& config);
    bool load_tileindex(Configuration& config);
    bool load_maptiles(Configuration& config);
    bool load_objtiles(Configuration& config);
    bool load_animmask(Configuration& config);

    void draw_transparent_tile(uint8_t* dst, int ypitch, uint8_t* src);
    void draw_plain_tile(uint8_t* dst, int ypitch, uint8_t* src);
    void draw_pixelblock_tile(uint8_t* dst, int ypitch, uint8_t* src);

private:
    std::vector<uint8_t>        m_masktype;
    std::vector<uint16_t>       m_tileindex;
    std::vector<uint8_t>        m_maptiles;
    std::vector<uint8_t>        m_objtiles;
};
