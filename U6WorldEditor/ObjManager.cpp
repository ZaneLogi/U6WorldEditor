#include "stdafx.h"
#include <fstream>
#include <cassert>
#include <vector>
#include "ObjManager.h"

const int WORLD_TILES = 1024;
const int DUNGEON_TILES = 256;

bool ObjManager::init(Configuration& config, TileManager* tile_manager)
{
    m_tile_manager = tile_manager;
    load_objblk(config);
    load_objlist(config);
    return true;
}

void ObjManager::draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile)
{
    assert(z <= 5 && "z is out of range!");
    assert((z == 0 ? world_tile_x < 1024 : world_tile_x < 256) && "world_tile_x is out of range!");
    assert((z == 0 ? world_tile_y < 1024 : world_tile_y < 256) && "world_tile_y is out of range!");
    assert(ds.width() % 16 == 0 && "must be 16x!");
    assert(ds.height() % 16 == 0 && "must be 16x!");

    int xstart = world_tile_x;
    int ystart = world_tile_y;
    int xend = xstart + ds.width() / 16;
    int yend = ystart + ds.height() / 16;

    if (z == 0)
    {
        for (int row = 0; row < 8; row++)
        {
            int top = row * 128;
            int bottom = top + 128;
            for (int col = 0; col < 8; col++)
            {
                int left = col * 128;
                int right = left + 128;
                if (xstart >= right || xend < left
                    || ystart >= bottom || yend < top)
                    continue; // out of range

                auto obj_itr = m_surface_objs[row][col].end();
                auto count = m_surface_objs[row][col].size();
                while (count-- > 0)
                {
                    --obj_itr;

                    const int WORLD_TILES = 1024;

                    int xtile1 = obj_itr->x;
                    int ytile1 = obj_itr->y;
                    int xtile2 = xtile1 + WORLD_TILES;
                    int ytile2 = ytile1 + WORLD_TILES;

                    int xtile, ytile;

                    if (xstart <= xtile1 && xtile1 < xend)
                    {
                        xtile = xtile1 - world_tile_x;
                    }
                    else if (xstart <= xtile2 && xtile2 < xend)
                    {
                        xtile = xtile2 - world_tile_x; // rewind in the horizonal
                    }
                    else
                    {
                        xtile = -1;
                    }

                    if (ystart <= ytile1 && ytile1 < yend)
                    {
                        ytile = ytile1 - world_tile_y;
                    }
                    else if (ystart <= ytile2 && ytile2 < yend)
                    {
                        ytile = ytile2 - world_tile_y; // rewind in the vertical
                    }
                    else
                    {
                        ytile = -1;
                    }

                    if (xtile >= 0 && ytile >= 0)
                    {
                        m_tile_manager->draw(ds, xtile * 16, ytile * 16,
                            obj_itr->obj_number, obj_itr->obj_frame, toptile);
                    }
                }
            }
        }
    }
    else
    {
        z--;
        auto obj_itr = m_dungeon_objs[z].end();
        auto count = m_dungeon_objs[z].size();
        while (count-- > 0)
        {
            --obj_itr;

            const int DUNGEON_TILES = 256;

            int xtile1 = obj_itr->x;
            int ytile1 = obj_itr->y;
            int xtile2 = xtile1 + DUNGEON_TILES;
            int ytile2 = ytile1 + DUNGEON_TILES;

            int xtile, ytile;

            // check if the object is inside the viewport
            if (xstart <= xtile1 && xtile1 < xend)
            {
                xtile = xtile1 - world_tile_x;
            }
            else if (xstart <= xtile2 && xtile2 < xend)
            {
                xtile = xtile2 - world_tile_x; // rewind in the horizonal
            }
            else
            {
                xtile = -1;
            }

            if (ystart <= ytile1 && ytile1 < yend)
            {
                ytile = ytile1 - world_tile_y;
            }
            else if (ystart <= ytile2 && ytile2 < yend)
            {
                ytile = ytile2 - world_tile_y; // rewind in the vertical
            }
            else
            {
                ytile = -1;
            }

            if (xtile >= 0 && ytile >= 0)
            {
                m_tile_manager->draw(ds, xtile * 16, ytile * 16,
                    obj_itr->obj_number, obj_itr->obj_frame, toptile);
            }
        }
    }

    draw_actors(ds, world_tile_x, world_tile_y, z, toptile);
}

Obj*  ObjManager::get_obj(uint16_t xtile, uint16_t ytile, uint8_t z)
{
    // check actors first
    for (int i = 0; i < 256; i++)
    {
        auto& actor = m_actors[i];
        if (actor.x == xtile && actor.y == ytile && actor.z == z)
        {
            return &actor;
        }
    }

    // check objects next
    std::list<Obj>* objs;
    if (z == 0)
    {
        objs = &m_surface_objs[ytile / 128][xtile / 128];
    }
    else
    {
        objs = &m_dungeon_objs[z - 1];
    }

    auto obj_itr = objs->begin();
    auto obj_end = objs->end();

    for (; obj_itr != obj_end; ++obj_itr)
    {
        TileInfo ti = m_tile_manager->get_info(obj_itr->obj_number, obj_itr->obj_frame);

        if (obj_itr->x == xtile && obj_itr->y == ytile)
            return &(*obj_itr);

        if (ti.double_width && obj_itr->x - 1 == xtile && obj_itr->y == ytile)
            return &(*obj_itr);

        if (ti.double_height && obj_itr->x == xtile && obj_itr->y - 1 == ytile)
            return &(*obj_itr);

        if (ti.double_width && ti.double_height && obj_itr->x - 1 == xtile && obj_itr->y - 1 == ytile)
            return &(*obj_itr);
    }

    return nullptr;
}

bool ObjManager::load_objblk(Configuration& config)
{
    auto objblk = config.get_path("objblk");
    //
    // get OBJBLK?? surface
    //
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            std::string filename = objblk + (char)('A' + col) + (char)('A' + row);

            std::ifstream objblk_file(filename, std::ios_base::in | std::ios_base::binary);
            if (!objblk_file.is_open())
            {
                assert(false && "Couldn't open source file OBJBLK!");
                return false;
            }

            load_superchunk(objblk_file, m_surface_objs[row][col]);
        }
    }

    // get dungeon
    for (int d = 0; d < 5; d++)
    {
        std::string filename = objblk + (char)('A' + d) + 'I';

        std::ifstream objblk_file(filename, std::ios_base::in | std::ios_base::binary);
        if (!objblk_file.is_open())
        {
            assert(false && "Couldn't open source file OBJBLK!");
            return false;
        }

        load_superchunk(objblk_file, m_dungeon_objs[d]);
    }

    return true;
}

void ObjManager::load_superchunk(std::istream& is, std::list<Obj>& objlist)
{
    objlist.clear();

    int obj_count = is.get() | (is.get() << 8);
    std::vector<Obj*> objrefs(obj_count);

    FileObjInfo info;

    for (int i = 0; i < obj_count; i++)
    {
        is.read((char*)&info, sizeof(info));

        Obj obj(info);

        if (obj.in_container())
        {
            int container = obj.container();
            auto container_obj = objrefs[container];
            container_obj->obj_list.push_back(obj);
            objrefs[i] = &container_obj->obj_list.back();
        }
        else if (obj.in_inventory())
        {
            int owner = obj.owner();
            m_actors[owner].obj_list.push_back(obj);
            objrefs[i] = &m_actors[owner].obj_list.back();
        }
        else
        {
            objlist.push_back(obj);
            objrefs[i] = &objlist.back();
        }

    }

}

bool ObjManager::load_objlist(Configuration& config)
{
    auto objlist_filename = config.get_path("objlist");

    std::ifstream objlist_file(objlist_filename, std::ios_base::in | std::ios_base::binary);
    if (!objlist_file.is_open())
    {
        assert(false && "Can't open file OBJLIST!");
        return false;
    }

    objlist_file.seekg(0x100); // Start of Actor position info

    // get x, y, z
    for (int i = 0; i < 256; i++)
    {
        int b1 = objlist_file.get();
        int b2 = objlist_file.get();
        int b3 = objlist_file.get();
        m_actors[i].x = (((b2 & 0x03) << 8) | b1);                  // 10bits = 0 - 1023
        m_actors[i].y = (((b3 & 0x0f) << 6) | ((b2 & 0xfc) >> 2));  // 10bits = 0 - 1023
        m_actors[i].z = ((b3 & 0xf0) >> 4);                         // 4bits = 0 - 15
    }

    // get obj information
    for (int i = 0; i < 256; i++)
    {
        int b1 = objlist_file.get();
        int b2 = objlist_file.get();
        m_actors[i].obj_number = (((b2 & 0x03) << 8) | b1); // 10bits = 0 - 1023
        m_actors[i].obj_frame = ((b2 & 0xfc) >> 2);         // 6bits = 0 - 63
        // the actor's direction = m_actors[i].obj_frame / 4
    }

    // Strength
    objlist_file.seekg(0x900);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].strength = objlist_file.get();
    }

    // Dexterity
    objlist_file.seekg(0xa00);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].dexterity = objlist_file.get();
    }

    // Intelligence
    objlist_file.seekg(0xb00);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].intelligence = objlist_file.get();
    }

    // Experience
    objlist_file.seekg(0xc00);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].exp = (objlist_file.get() + objlist_file.get() * 256);
    }

    // Health
    objlist_file.seekg(0xe00);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].hp = objlist_file.get();
    }

    // Level
    objlist_file.seekg(0xff1);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].level = objlist_file.get();
    }

    // Magic Points
    objlist_file.seekg(0x13f1);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].mp = objlist_file.get();
    }

    // Start of Actor flags
    objlist_file.seekg(0x17f1);
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].flags = objlist_file.get();
    }


    objlist_file.close();
    return true;
}

void ObjManager::draw_actors(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile)
{
    int xstart = world_tile_x;
    int ystart = world_tile_y;
    int xend = xstart + ds.width() / 16;
    int yend = ystart + ds.height() / 16;
    int tiles_count;

    if (z == 0)
    {
        tiles_count = WORLD_TILES;
    }
    else
    {
        tiles_count = DUNGEON_TILES;
    }

    // draw actors
    for (int i = 0; i < 256; i++)
    {
        auto& actor = m_actors[i];
        if (actor.obj_number == 0 || actor.z != z)
            continue;

        int xtile1 = actor.x;
        int ytile1 = actor.y;
        int xtile2 = xtile1 + tiles_count;
        int ytile2 = ytile1 + tiles_count;

        int xtile, ytile;

        if (xstart <= xtile1 && xtile1 < xend)
        {
            xtile = xtile1 - world_tile_x;
        }
        else if (xstart <= xtile2 && xtile2 < xend)
        {
            xtile = xtile2 - world_tile_x; // rewind in the horizonal
        }
        else
        {
            xtile = -1;
        }

        if (ystart <= ytile1 && ytile1 < yend)
        {
            ytile = ytile1 - world_tile_y;
        }
        else if (ystart <= ytile2 && ytile2 < yend)
        {
            ytile = ytile2 - world_tile_y; // rewind in the vertical
        }
        else
        {
            ytile = -1;
        }

        if (xtile >= 0 && ytile >= 0)
        {
            m_tile_manager->draw(ds, xtile * 16, ytile * 16,
                actor.obj_number, actor.obj_frame, toptile);
        }
    }
}

