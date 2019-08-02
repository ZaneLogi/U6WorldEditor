#pragma once
#include <vector>
#include "Configuration.h"
#include "DibSection.h"
#include "TileImage.h"

#pragma pack(1)
struct AnimData
{
    uint16_t    number_of_tiles_to_animate;
    uint16_t    tile_to_animate[0x20];
    uint16_t    first_anim_frame[0x20];
    uint8_t     and_masks[0x20];
    uint8_t     shift_values[0x20];
};
#pragma pack()

struct TileInfo
{
    uint16_t    index;
    bool        top;
    bool        double_width;
    bool        double_height;
};

class TileManager
{
public:
    TileManager();
    ~TileManager();

    bool init(Configuration& config, TileImage* tile_image);
    void update();
    void draw(DibSection& ds, int x, int y, uint16_t tile_index);
    void draw(DibSection& dc, int x, int y, uint16_t obj_number, uint8_t obj_frame, bool toptile);
    std::string get_description(uint16_t tile_index);
    std::string get_description(uint16_t obj_number, uint8_t obj_frame, uint8_t quantity);
    TileInfo get_info(uint16_t obj_number, uint8_t obj_frame);

private:
    bool load_animdata(Configuration& config);  // => m_animdata
    bool load_basetile(Configuration& config);  // => m_obj_to_tile
    bool load_tileflag(Configuration& config);  // => m_tile_flags1/2/3
    bool load_look(Configuration& config);      // => m_look_data

private:
    TileImage*  m_tile_image;
    uint32_t    m_anim_counter = 0;
    uint16_t    m_anim_tile_index[2048];
    AnimData    m_animdata;                 // it is used for the tile animations.
    uint16_t    m_obj_to_tile[1024];        // it is used for the mappings from the object index to the tile index

    uint8_t     m_tile_flags1[2048];
    uint8_t     m_tile_flags2[2048];
    uint8_t     m_tile_flags3[2048];

    std::vector<uint8_t> m_look_data;       // it is used for the names of the objects
    uint8_t*    m_tile_look[2048];
};

