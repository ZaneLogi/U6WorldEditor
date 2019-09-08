#pragma once
#include <fstream>
#include <list>
#include "DibSection.h"
#include "Configuration.h"
#include "TileManager.h"

// obj status bit flags
#define OBJ_STATUS_OK_TO_TAKE    0x1
#define OBJ_STATUS_SEEN_EGG      0x2  // something to do wih eggs
//#define OBJ_STATUS_UNKNOWN     0x4
#define OBJ_STATUS_IN_CONTAINER  0x8
#define OBJ_STATUS_IN_INVENTORY 0x10
#define OBJ_STATUS_TEMPORARY    0x20
#define OBJ_STATUS_EGG_ACTIVE   0x40  // something to do with eggs
//#define OBJ_STATUS_UNKNOWN    0x80
// combined:
#define OBJ_STATUS_READIED      0x18


#pragma pack(1)
struct FileObjInfo
{
    uint32_t status : 8;
    uint32_t x : 10;
    uint32_t y : 10;
    uint32_t z : 4;

    uint32_t obj_number : 10;
    uint32_t obj_frame : 6;
    uint32_t quantity : 8;
    uint32_t quality : 8;
};
#pragma pack()

class Obj
{
public:
    Obj() = default;
    Obj(const FileObjInfo& info) : status(info.status), x(info.x), y(info.y), z(info.z),
                                   obj_number(info.obj_number), obj_frame(info.obj_frame),
                                   quantity(info.quantity), quality(info.quality)
    {}

    bool operator == (const Obj& obj) {
        return this->x == obj.x && this->y == obj.y && this->z == obj.z &&
            this->obj_number == obj.obj_number &&
            this->quantity == obj.quantity && this->quality && obj.quality;
    }

    bool in_container() const {
        // bit 3: contained (as long as bit 4, in_npc, is clear)
        return ((status & OBJ_STATUS_READIED) == OBJ_STATUS_IN_CONTAINER);
    }

    int container() const {
        return (x | ((y & 0x3) << 10));
    }

    bool in_inventory() const {
        // Note: includes readied items.
        return ((status & OBJ_STATUS_IN_INVENTORY) != 0);
    }

    int owner() const {
        // x-coord stores NPC number (0-255).
        return x;
    }

    bool is_readied() const {
        // Container bit is overloaded for readied items.
        return ((status & OBJ_STATUS_READIED) == OBJ_STATUS_READIED);
    }

    enum class Type : uint8_t { OBJ, ACTOR };
    virtual Type type() const { return Type::OBJ; }

public:
    uint8_t         status;
    uint16_t        x;  // tiles, surface: 0 - 1023, dungeon: 0 - 255
    uint16_t        y;  // tiles, surface: 0 - 1023, dungeon: 0 - 255
    uint8_t         z;  // levels, surface: 0, dungeon: 1 - 5

    uint16_t        obj_number;
    uint8_t         obj_frame;

    uint8_t         quantity;
    uint8_t         quality;

    TileInfo        tile_info;

    std::list<Obj>  obj_list;
};

class Actor : public Obj
{
public:
    uint8_t     id;
    std::string name;

    uint8_t*    strength;
    uint8_t*    dexterity;
    uint8_t*    intelligence;
    uint8_t*    hp;
    uint8_t*    mp;
    uint8_t*    level;
    uint16_t*   exp;
    uint8_t*    flags;

    virtual Type type() const { return Type::ACTOR; }
    virtual int max_hp()
    {
        // savage empire
        return(((*level * 4 + *strength * 2) < 255) ? (*level * 4 + *strength * 2) : 255);
        // martian dream
        //return(((*level * 24 + *strength * 2) < 255) ? (*level * 24 + *strength * 2) : 255);
        // u6
        //return (((*level * 30) <= 255) ? (*level * 30) : 255);
    }
};

class ObjManager
{
public:
    ObjManager() = default;

    bool init(Configuration& config, TileManager* tile_manager);
    void draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z);

    Actor get_actor(int index) const { return m_actors[index]; }
    void set_actor_position(int index, const uint8_t* data);
    Obj*  get_obj(uint16_t xtile, uint16_t ytile, uint8_t z);

    bool move_obj_to(const Obj& obj, uint16_t xtile, uint16_t ytile, uint8_t z);


    bool save_objblks(const char* folder);
    bool save_superchunk(const char* filename, int col, int row, int z);
    bool save_actors(const char* filename);

private:
    bool load_objblk(Configuration& config);
    void load_superchunk(std::istream& is, std::list<Obj>& objlist);
    bool load_objlist(Configuration& config);
    void init_z_order(Configuration& config);

    void cache_objs_in_superchunks(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, int super_x, int super_y, const std::list<Obj>& objs, bool toptile);
    void cache_actors(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile);

private:
    TileManager*    m_tile_manager;

    std::list<Obj>          m_surface_objs[8][8];
    std::list<Obj>          m_dungeon_objs[5];
    std::vector<uint8_t>    m_objlist;
    Actor                   m_actors[256];
    std::vector<Actor*>     m_party_members;

    int                     m_obj_z_order[1536]; // this is used for the big flat object drawing, especially at Ultima6 (52,172,1) and the mat in SE

    class display_cache
    {
    public:
        display_cache()
        {
            std::for_each(m_display_objs, m_display_objs + 1024, [](auto& i) {i.resize(16); });
        }
        void reset()
        {
            std::for_each(m_display_count, m_display_count + 1024, [](int& n) {n = 0; });
        }
        void append_obj(int xtile, int ytile, const Obj* obj)
        {
            auto& count = m_display_count[obj->y];
            auto& objs = m_display_objs[obj->y];
            auto& c = objs[count++];
            c.xtile = xtile;
            c.ytile = ytile;
            c.obj = obj;
            if (count >= objs.size())
            {
                objs.resize(count + count);
            }
        }
    public:
        struct cache_data { uint16_t xtile; uint16_t ytile; const Obj* obj; };

        std::vector<cache_data> m_display_objs[1024];
        int                     m_display_count[1024];
    } m_display_cache;
};