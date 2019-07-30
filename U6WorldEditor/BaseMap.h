#pragma once
#include <fstream>
#include "DibSection.h"
#include "Configuration.h"
#include "TileManager.h"

class BaseMap
{
public:
    BaseMap();
    ~BaseMap();

    bool init(Configuration& config, TileManager* tile_manager);
    void draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z);

private:
    bool load_chunks(Configuration& config);
    bool load_map(Configuration& config);
    void create_superchunk(std::istream& is, int sx, int sy);
    void create_dungeon(std::istream& is, int level);

private:
    TileManager*    m_tile_manager;

    bool            m_is_u6;
    // the world consists of 1024 x 1024 tiles
    // each dungeon consists of 256 x 256 tiles
    uint8_t         m_chunks[1024][8][8];
    uint16_t        m_superchunks[128][128]; // [128][128] = [8][8][16][16]
    uint16_t        m_dungeon_chunks[5][32][32]; // [1024] = [32][32]
};

