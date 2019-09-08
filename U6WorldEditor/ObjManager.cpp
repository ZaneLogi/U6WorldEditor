#include "stdafx.h"
#include <fstream>
#include <cassert>
#include <vector>
#include <set>
#include "ObjManager.h"

const int WORLD_TILES = 1024;
const int DUNGEON_TILES = 256;

inline uint32_t FILESIZE(std::ifstream& f)
{
    auto cur_pos = f.tellg();
    f.seekg(0, std::ios::end);
    auto fsize = f.tellg();
    f.seekg(cur_pos, std::ios::beg);
    return (uint32_t)fsize;
}



bool ObjManager::init(Configuration& config, TileManager* tile_manager)
{
    m_tile_manager = tile_manager;
    load_objblk(config);
    load_objlist(config);
    init_z_order(config);
    return true;
}

void ObjManager::draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z)
{
    assert(z <= 5 && "z is out of range!");
    assert((z == 0 ? world_tile_x < 1024 : world_tile_x < 256) && "world_tile_x is out of range!");
    assert((z == 0 ? world_tile_y < 1024 : world_tile_y < 256) && "world_tile_y is out of range!");
    assert(ds.width() % 16 == 0 && "must be 16x!");
    assert(ds.height() % 16 == 0 && "must be 16x!");

    m_display_cache.reset();

    if (z == 0)
    {
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                cache_objs_in_superchunks(ds, world_tile_x, world_tile_y, z, col, row, m_surface_objs[row][col], false);
            }
        }
    }
    else
    {
        cache_objs_in_superchunks(ds, world_tile_x, world_tile_y, z, 0, 0, m_dungeon_objs[z - 1], false);
    }

    cache_actors(ds, world_tile_x, world_tile_y, z, false);

    // draw bottom tiles first
    int world_tiles = (z == 0) ? WORLD_TILES : DUNGEON_TILES;
    int lines = ds.height() / 16;
    for (int i = 0; i < lines; i++)
    {
        int y = (world_tile_y + i) % world_tiles;
        auto obj_itr = m_display_cache.m_display_objs[y].begin();
        auto count = m_display_cache.m_display_count[y];
        for (int j = 0; j < count; j++, ++obj_itr)
        {
            auto& o = *obj_itr;
            m_tile_manager->draw(ds, o.xtile * 16, o.ytile * 16,
                o.obj->obj_number, o.obj->obj_frame, false);

            int obj_z_order = m_obj_z_order[o.obj->obj_number];

            bool double_width, double_height;
            bool big_flat_obj = m_tile_manager->is_big_flat_object(o.obj->obj_number, o.obj->obj_frame, double_width, double_height);
            if (big_flat_obj)
            {
                int prev_y = y - 1;
                if (prev_y < 0)
                    prev_y += world_tiles;

                if (double_width)
                {
                    // draw the objects again
                    auto search_itr = m_display_cache.m_display_objs[y].begin();
                    auto search_count = m_display_cache.m_display_count[y];
                    int target_x = o.obj->x - 1;
                    int target_y = o.obj->y;
                    for (int s = 0; s < search_count; s++, ++search_itr)
                    {
                        if (search_itr->obj->x == target_x && m_obj_z_order[search_itr->obj->obj_number] > obj_z_order)
                        {
                            m_tile_manager->draw(ds, search_itr->xtile * 16, search_itr->ytile * 16,
                                search_itr->obj->obj_number, search_itr->obj->obj_frame, false);
                        }
                    }
                }

                if (double_height)
                {
                    // draw the objects again
                    auto search_itr = m_display_cache.m_display_objs[prev_y].begin();
                    auto search_count = m_display_cache.m_display_count[prev_y];
                    int target_x = o.obj->x;
                    int target_y = o.obj->y - 1;
                    for (int s = 0; s < search_count; s++, ++search_itr)
                    {
                        if (search_itr->obj->x == target_x && m_obj_z_order[search_itr->obj->obj_number] > obj_z_order)
                        {
                            m_tile_manager->draw(ds, search_itr->xtile * 16, search_itr->ytile * 16,
                                search_itr->obj->obj_number, search_itr->obj->obj_frame, false);
                        }
                    }
                }

                if (double_width && double_height)
                {
                    // draw the objects again
                    auto search_itr = m_display_cache.m_display_objs[prev_y].begin();
                    auto search_count = m_display_cache.m_display_count[prev_y];
                    int target_x = o.obj->x - 1;
                    int target_y = o.obj->y - 1;
                    for (int s = 0; s < search_count; s++, ++search_itr)
                    {
                        if (search_itr->obj->x == target_x && m_obj_z_order[search_itr->obj->obj_number] > obj_z_order)
                        {
                            m_tile_manager->draw(ds, search_itr->xtile * 16, search_itr->ytile * 16,
                                search_itr->obj->obj_number, search_itr->obj->obj_frame, false);
                        }
                    }
                }
            }
        }
    }

    // draw top tiles later
    for (int i = 0; i < lines; i++)
    {
        int y = (world_tile_y + i) % world_tiles;
        auto obj_itr = m_display_cache.m_display_objs[y].begin();
        auto count = m_display_cache.m_display_count[y];
        for (int j = 0; j < count; j++, ++obj_itr)
        {
            auto& o = *obj_itr;
            m_tile_manager->draw(ds, o.xtile * 16, o.ytile * 16,
                o.obj->obj_number, o.obj->obj_frame, true);
        }
    }
}

void ObjManager::set_actor_position(int index, const uint8_t* data)
{
    auto actor = &m_actors[index];
    int b1 = *data++;
    int b2 = *data++;
    int b3 = *data++;
    actor->x = (((b2 & 0x03) << 8) | b1);                  // 10bits = 0 - 1023
    actor->y = (((b3 & 0x0f) << 6) | ((b2 & 0xfc) >> 2));  // 10bits = 0 - 1023
    actor->z = ((b3 & 0xf0) >> 4);                         // 4bits = 0 - 15
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

    //
    // TODO: the pick is incorrect at Ultima6 (52,172,1)
    // it is not enough to only check (xtile, ytile),
    // it should also check (xtile+1, ytile+1) for the double-size objects
    //

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
        auto& ti = obj_itr->tile_info;

        if (obj_itr->x == xtile && obj_itr->y == ytile)
            return &(*obj_itr);

        if (ti.double_width() && obj_itr->x - 1 == xtile && obj_itr->y == ytile)
            return &(*obj_itr);

        if (ti.double_height() && obj_itr->x == xtile && obj_itr->y - 1 == ytile)
            return &(*obj_itr);

        if (ti.double_width() && ti.double_height() && obj_itr->x - 1 == xtile && obj_itr->y - 1 == ytile)
            return &(*obj_itr);
    }

    return nullptr;
}

bool ObjManager::move_obj_to(const Obj& obj, uint16_t xtile, uint16_t ytile, uint8_t z)
{
    // check actors first
    for (int i = 0; i < 256; i++)
    {
        auto& actor = m_actors[i];
        if (actor.x == obj.x && actor.y == obj.y && actor.z == obj.z)
        {
            actor.x = xtile;
            actor.y = ytile;
            actor.z = z;
            return true;
        }
    }

    // check objects next
    std::list<Obj>* objs;
    if (obj.z == 0)
    {
        objs = &m_surface_objs[obj.y / 128][obj.x / 128];
    }
    else
    {
        objs = &m_dungeon_objs[obj.z - 1];
    }

    auto obj_itr = objs->begin();
    auto obj_end = objs->end();

    for (; obj_itr != obj_end; ++obj_itr)
    {
        if (obj_itr->x == obj.x && obj_itr->y == obj.y)
            break;
    }

    if (obj_itr == obj_end)
        return false; // found no object

    Obj target = *obj_itr;
    objs->erase(obj_itr);

    // move to
    if (z == 0)
    {
        objs = &m_surface_objs[ytile / 128][xtile / 128];
    }
    else
    {
        objs = &m_dungeon_objs[z - 1];
    }

    target.x = xtile;
    target.y = ytile;
    target.z = z;

    obj_itr = objs->begin();
    obj_end = objs->end();

    for (; obj_itr != obj_end; ++obj_itr)
    {
        if ((obj_itr->y == target.y && obj_itr->x >= target.x) ||
            (obj_itr->y > target.y))
        {
            // insert here,
            // if there are other objects in the same location, the new inserted object will be on top of them.
            objs->insert(obj_itr, target);
            return true;
        }
    }

    objs->push_back(target);
    return true;
}

bool ObjManager::save_objblks(const char* folder)
{
    // surface
    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            std::string filename(folder);
            filename += std::string("\\OBJBLK") + (char)('A' + col) + (char)('A' + row);
            save_superchunk(filename.c_str(), col, row, 0);
        }
    }

    // dungeons
    for (int z = 0; z < 5; z++)
    {
        std::string filename(folder);
        filename += std::string("\\OBJBLK") + (char)('A' + z) + 'I';
        save_superchunk(filename.c_str(), 0, 0, z + 1);
    }

    // actors
    {
        std::string filename(folder);
        filename += "\\OBJLIST";
        save_actors(filename.c_str());
    }

    return true;
}

bool ObjManager::save_superchunk(const char* filename, int col, int row, int z)
{
    assert(0 <= col && col < 8);
    assert(0 <= row && row < 8);
    assert(0 <= z && z <= 5);
    int xstart = 0;
    int ystart = 0;
    int xend = 0;
    int yend = 0;

    std::list<Obj>* objs;
    if (z == 0)
    {
        objs = &m_surface_objs[row][col];
        xstart = col * 128;
        xend = xstart + 128;
        ystart = row * 128;
        yend = ystart + 128;
    }
    else
    {
        objs = &m_dungeon_objs[z - 1];
        xstart = 0;
        xend = 256;
        ystart = 0;
        yend = 256;
    }

    std::list<Obj*> refs;
    auto obj_itr = objs->begin();
    auto obj_end = objs->end();

    // objects
    for (; obj_itr != obj_end; ++obj_itr)
    {
        refs.push_back(&(*obj_itr));
    }

    // actors
    std::set<int> m_actor_set; // for debug
    for (int i = 0; i < 256; i++)
    {
        auto& actor = m_actors[i];
        if (actor.obj_number != 0 && actor.z == z &&
            xstart <= actor.x && actor.x < xend &&
            ystart <= actor.y && actor.y < yend)
        {
            m_actor_set.insert(i);
            // insert the actor to the list
            auto refs_itr = refs.begin();
            auto refs_end = refs.end();

            for (; refs_itr != refs_end; ++refs_itr)
            {
                if (((*refs_itr)->y == actor.y && (*refs_itr)->x >= actor.x) ||
                    ((*refs_itr)->y > actor.y))
                {
                    // insert here
                    refs.insert(refs_itr, &actor);
                    break;
                }
            }
            if (refs_itr == refs_end)
            {
                refs.push_back(&actor);
            }
        }
    }

    // save objects to the file
    std::ofstream file(filename, std::ios_base::out | std::ios_base::binary);
    if (!file.is_open())
        return false;

    uint16_t count = 0;
    file.write((char*)&count, sizeof(count));

    class file_oprator
    {
    public:
        // recursive function
        void save(std::ostream& os, const Obj* obj, uint16_t& current_index, uint16_t parent_index = -1, const Obj* parent = nullptr)
        {
            FileObjInfo info;

            auto type = obj->type();
            assert(type == Obj::Type::OBJ || type == Obj::Type::ACTOR);
            if (type == Obj::Type::OBJ)
            {
                info.status = obj->status;
                info.x = obj->x;
                info.y = obj->y;
                info.z = obj->z;
                info.obj_number = obj->obj_number;
                info.obj_frame = obj->obj_frame;
                info.quantity = obj->quantity;
                info.quality = obj->quality;

                if (parent != nullptr)
                {
                    if (parent->type() == Obj::Type::OBJ)
                    {
                        info.x = (parent_index & 0x3ff);
                        info.y = ((parent_index >> 10) & 0x03)|(info.y & 0x03fc);
                        // info.y & 03fc is used to make it same as the orignal game file
                    }
                    else if (parent->type() == Obj::Type::ACTOR)
                    {
                        assert(obj->in_inventory());
                        assert(info.x == ((Actor*)parent)->id);
                    }
                }

                os.write((char*)&info, sizeof(info));
                parent_index = current_index;
                current_index++;
            }

            for (auto i = obj->obj_list.begin(); i != obj->obj_list.end(); ++i)
            {
                save(os, &(*i), current_index, parent_index, obj);
            }
        }
    };

    auto refs_itr = refs.begin();
    auto refs_end = refs.end();
    for (; refs_itr != refs_end; ++refs_itr)
    {
        file_oprator().save(file, *refs_itr, count);
    }

    file.seekp(0);
    file.write((char*)&count, sizeof(count));
    file.close();

    return true;
}

bool ObjManager::save_actors(const char* filename)
{
    std::ofstream file(filename, std::ios_base::out | std::ios_base::binary);
    if (!file.is_open())
        return false;

    file.write((char*)m_objlist.data(), m_objlist.size());
    file.close();
    return true;
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

    std::set<int> m_actor_set; // for debug

    for (int i = 0; i < obj_count; i++)
    {
        is.read((char*)&info, sizeof(info));

        Obj obj(info);
        obj.tile_info = m_tile_manager->get_info(obj.obj_number, obj.obj_frame);

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
            m_actor_set.insert(owner);
            m_actors[owner].obj_list.push_back(obj);
            objrefs[i] = &m_actors[owner].obj_list.back();
        }
        else
        {
            objlist.push_back(obj);
            objrefs[i] = &objlist.back();
        }
    }

#if 0
    // DEBUG - check the number of the loaded objects is same as the number of objects in the file
    class Counter
    {
    public:
        int count(Obj* obj)
        {
            int n = 1;
            for (auto i = obj->obj_list.begin(); i != obj->obj_list.end(); ++i)
            {
                n += count(&*i);
            }
            return n;
        }
    };

    int total = 0;
    for (auto i = objlist.begin(); i != objlist.end(); ++i)
    {
        total += Counter().count(&(*i));
    }

    for (auto i = m_actor_set.begin(); i != m_actor_set.end(); ++i)
    {
        auto& actor = m_actors[*i];
        for (auto j = actor.obj_list.begin(); j != actor.obj_list.end(); ++j)
        {
            total += Counter().count(&(*j));
        }
    }
    assert(total == obj_count);
#endif
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

    auto fsize = FILESIZE(objlist_file);
    m_objlist.resize(fsize);
    objlist_file.read((char*)m_objlist.data(), fsize);
    objlist_file.close();

    auto p = m_objlist.data() + 0x100; // Start of Actor position info

    // get x, y, z
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].id = i;

        int b1 = *p++;
        int b2 = *p++;
        int b3 = *p++;
        m_actors[i].x = (((b2 & 0x03) << 8) | b1);                  // 10bits = 0 - 1023
        m_actors[i].y = (((b3 & 0x0f) << 6) | ((b2 & 0xfc) >> 2));  // 10bits = 0 - 1023
        m_actors[i].z = ((b3 & 0xf0) >> 4);                         // 4bits = 0 - 15
    }

    // get obj information
    for (int i = 0; i < 256; i++)
    {
        int b1 = *p++;
        int b2 = *p++;
        m_actors[i].obj_number = (((b2 & 0x03) << 8) | b1); // 10bits = 0 - 1023
        m_actors[i].obj_frame = ((b2 & 0xfc) >> 2);         // 6bits = 0 - 63
        // the actor's direction = m_actors[i].obj_frame / 4

        m_actors[i].tile_info = m_tile_manager->get_info(m_actors[i].obj_number, m_actors[i].obj_frame);
    }

    // Strength
    p = m_objlist.data() + 0x900;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].strength = p++;
    }

    // Dexterity
    p = m_objlist.data() + 0xa00;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].dexterity = p++;
    }

    // Intelligence
    p = m_objlist.data() + 0xb00;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].intelligence = p++;
    }

    // Experience
    p = m_objlist.data() + 0xc00;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].exp = (uint16_t*)p;
        p += 2;
    }

    // Health
    p = m_objlist.data() + 0xe00;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].hp = p++;
    }

    // Level
    p = m_objlist.data() + 0xff1;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].level = p++;
    }

    // Magic Points
    p = m_objlist.data() + 0x13f1;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].mp = p++;
    }

    // Start of Actor flags
    p = m_objlist.data() + 0x17f1;
    for (int i = 0; i < 256; i++)
    {
        m_actors[i].flags = p++;
    }

    // party
    auto party_member_count = *(m_objlist.data() + 0xff0);
    m_party_members.resize(party_member_count);
    auto member_name = m_objlist.data() + 0xf00;
    auto member_index = m_objlist.data() + 0xfe0;
    for (int i = 0; i < party_member_count; i++)
    {
        int actor_index = *(member_index + i);
        m_actors[actor_index].name = std::string((char*)(member_name + i * 14));
        m_party_members[i] = &m_actors[actor_index];
    }

    return true;
}

void ObjManager::init_z_order(Configuration& config)
{
    memset(m_obj_z_order, 0, sizeof(m_obj_z_order));

    auto game_type = config.get_property("game_type");
    if (game_type == "u6")
    {
        m_obj_z_order[32] = -2;     // floor
        m_obj_z_order[311] = -1;    // stone table
    }
    else if (game_type == "se")
    {
        m_obj_z_order[244] = -1;    // mat
        m_obj_z_order[245] = -1;    // skin
    }
    else if (game_type == "md")
    {

    }

}

void ObjManager::cache_objs_in_superchunks(
    DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z,
    int super_x, int super_y, const std::list<Obj>& objs, bool toptile)
{
    // set the valid range in the DibSectoin
    int xstart = world_tile_x;
    int ystart = world_tile_y;
    int xend = xstart + ds.width() / 16;
    int yend = ystart + ds.height() / 16;

    // set the range in the superchunk
    int top, bottom, left, right;
    int world_tiles;
    if (z == 0)
    {
        top = super_y * 128;
        bottom = top + 128;
        left = super_x * 128;
        right = left + 128;
        world_tiles = WORLD_TILES;
    }
    else
    {
        top = 0;
        bottom = DUNGEON_TILES;
        left = 0;
        right = DUNGEON_TILES;
        world_tiles = DUNGEON_TILES;
    }

    if (xstart >= right || xend < left ||
        ystart >= bottom || yend < top)
        return; // out of range, no overlap

    auto obj_itr = objs.begin();
    auto obj_end = objs.end();
    while (obj_itr != obj_end)
    {
        // draw objects from left to right then top to bottom
        // if objects are at the same locations, draw them from the last one to the first one in the list
        auto first_obj_itr = obj_itr;
        int curx = obj_itr->x;
        int cury = obj_itr->y;
        ++obj_itr;
        int count = 1;
        for (; obj_itr != obj_end; ++obj_itr, ++count)
        {
            if (curx != obj_itr->x || cury != obj_itr->y)
                break;
        }

        auto draw_obj_itr = obj_itr;
        while (count-- > 0)
        {
            --draw_obj_itr;

            int xtile1 = draw_obj_itr->x;
            int ytile1 = draw_obj_itr->y;
            int xtile2 = xtile1 + world_tiles;
            int ytile2 = ytile1 + world_tiles;

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
                m_display_cache.append_obj(xtile, ytile, &(*draw_obj_itr));
            }
        }
    }
}

void ObjManager::cache_actors(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile)
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
            m_display_cache.append_obj(xtile, ytile, &actor);
        }
    }
}

