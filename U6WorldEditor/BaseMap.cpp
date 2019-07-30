#include "stdafx.h"
#include <cassert>
#include "BaseMap.h"

static BYTE U6_ANIM_SRC_TILE[32] =
{
    0x16,0x16,0x1a,0x1a,0x1e,0x1e,0x12,0x12,
    0x1a,0x1e,0x16,0x12,0x16,0x1a,0x1e,0x12,
    0x1a,0x1e,0x1e,0x12,0x12,0x16,0x16,0x1a,
    0x12,0x16,0x1e,0x1a,0x1a,0x1e,0x12,0x16
};

BaseMap::BaseMap()
{
}


BaseMap::~BaseMap()
{
}

bool BaseMap::init(Configuration& config, TileManager* tile_manager)
{
    m_tile_manager = tile_manager;

    m_is_u6 = (config.get_property("game_type") == "u6");

    load_chunks(config);
    load_map(config);
    return true;
}

void BaseMap::draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z)
{
    assert(z <= 5 && "z is out of range!");
    assert((z == 0 ? world_tile_x < 1024 : world_tile_x < 256) && "world_tile_x is out of range!");
    assert((z == 0 ? world_tile_y < 1024 : world_tile_y < 256) && "world_tile_y is out of range!");
    assert(ds.width()  % 16 == 0 && "must be 16x!");
    assert(ds.height() % 16 == 0 && "must be 16x!");

    int xstart = world_tile_x;
    int ystart = world_tile_y;
    int xend = xstart + ds.width() / 16;
    int yend = ystart + ds.height() / 16;

    if (z == 0)
    {
        const int WORLD_TILES = 1024;

        for (int ytile = ystart, cury = 0; ytile < yend; ytile++, cury += 16)
        {
            for (int xtile = xstart, curx = 0; xtile < xend; xtile++, curx += 16)
            {
                int x_coord = xtile % WORLD_TILES;
                int y_coord = ytile % WORLD_TILES;

                int chunk_index = m_superchunks[y_coord >> 3][x_coord >> 3];
                assert(chunk_index<1024);

                int tile_index = m_chunks[chunk_index][y_coord & 0x7][x_coord & 0x7];
                assert(tile_index<2048);

                if (m_is_u6 && tile_index >= 16 && tile_index < 48) //lay down the base tile for shoreline tiles
                {
                    m_tile_manager->draw(ds, curx, cury, U6_ANIM_SRC_TILE[tile_index - 16] / 2);
                }

                m_tile_manager->draw(ds, curx, cury, tile_index);
            }
        }
    }
    else
    {
        const int DUNGEON_TILES = 256;
        z--;

        for (int ytile = ystart, cury = 0; ytile < yend; ytile++, cury += 16)
        {
            for (int xtile = xstart, curx = 0; xtile < xend; xtile++, curx += 16)
            {
                int x_coord = xtile % DUNGEON_TILES;
                int y_coord = ytile % DUNGEON_TILES;

                int chunk_index = m_dungeon_chunks[z][y_coord >> 3][x_coord >> 3];
                assert(chunk_index<1024);

                int tile_index = m_chunks[chunk_index][y_coord & 0x7][x_coord & 0x7];
                assert(tile_index<2048);

                if (m_is_u6 && tile_index >= 16 && tile_index < 48) //lay down the base tile for shoreline tiles
                {
                    m_tile_manager->draw(ds, curx, cury, U6_ANIM_SRC_TILE[tile_index - 16] / 2);
                }

                m_tile_manager->draw(ds, curx, cury, tile_index);
            }
        }
    }
}

bool BaseMap::load_chunks(Configuration& config)
{
    auto chunks_filename = config.get_path("chunks");
    //
    // loading chunks
    //
    std::ifstream chunks_file(chunks_filename, std::ios_base::in | std::ios_base::binary);
    if (!chunks_file.is_open())
    {
        assert(false && "Couldn't open source file CHUNKS!");
        return false;
    }

    chunks_file.read((char*)&m_chunks[0][0][0], 65536);
    chunks_file.close();
    return true;
}

bool BaseMap::load_map(Configuration& config)
{
    auto map_filename = config.get_path("map");

    //
    // loading MAP
    //
    std::ifstream map_file(map_filename, std::ios_base::in | std::ios_base::binary);
    if (!map_file.is_open())
    {
        assert(false && "Couldn't open source file MAP!");
        return false;
    }

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            create_superchunk(map_file, col, row);
        }
    }

    //
    // loading dungeons
    //
    for (int i = 0; i < 5; i++) {
        create_dungeon(map_file, i);
    }

    return true;
}

void BaseMap::create_superchunk(std::istream& is, int sx, int sy)
{
    BYTE info[3];

    sx *= 16;
    int xtile = sx;
    int ytile = sy * 16;

    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x += 2)
        {
            // 12 50 34
            //
            // 012 345
            is.read((char*)info, sizeof(info));

            uint16_t index1 = info[0];
            index1 += (uint16_t)((info[1] & 0x0f) * 256);

            uint16_t index2 = (uint16_t)(info[2] << 4);
            index2 |= (uint16_t)(info[1] >> 4);

            m_superchunks[ytile][xtile++] = index1;
            m_superchunks[ytile][xtile++] = index2;
        }
        ytile++;
        xtile = sx;
    }
}

void BaseMap::create_dungeon(std::istream& is, int level)
{
    BYTE info[3];
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x += 2)
        {
            // 12 50 34
            //
            // 012 345
            is.read((char*)info, sizeof(info));

            uint16_t index1 = info[0];
            index1 += (uint16_t)((info[1] & 0x0f) * 256);

            uint16_t index2 = (uint16_t)(info[2] << 4);
            index2 |= (uint16_t)(info[1] >> 4);

            m_dungeon_chunks[level][y][x] = index1;
            m_dungeon_chunks[level][y][x + 1] = index2;
        }
    }
}
