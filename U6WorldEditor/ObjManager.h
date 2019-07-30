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

    bool in_container() {
        // bit 3: contained (as long as bit 4, in_npc, is clear)
        return ((status & OBJ_STATUS_READIED) == OBJ_STATUS_IN_CONTAINER);
    }

    int container() {
        return (x | ((y & 0x3) << 10));
    }

    bool in_inventory() {
        // Note: includes readied items.
        return ((status & OBJ_STATUS_IN_INVENTORY) != 0);
    }

    int owner() {
        // x-coord stores NPC number (0-255).
        return x;
    }

    bool is_readied() {
        // Container bit is overloaded for readied items.
        return ((status & OBJ_STATUS_READIED) == OBJ_STATUS_READIED);
    }

public:
    uint8_t         status;
    uint16_t        x;
    uint16_t        y;
    uint8_t         z;

    uint16_t        obj_number;
    uint8_t         obj_frame;

    uint8_t         quantity;
    uint8_t         quality;

    std::list<Obj>  obj_list;
};

class Actor : public Obj
{
public:
    uint8_t     strength;
    uint8_t     dexterity;
    uint8_t     intelligence;
    uint8_t     hp;
    uint8_t     mp;
    uint8_t     level;
    uint16_t    exp;
    uint8_t     flags;
};

class ObjManager
{
public:
    bool init(Configuration& config, TileManager* tile_manager);
    void draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile);

    Actor get_actor(int index) const { return m_actors[index]; }
    Obj*  get_obj(uint16_t xtile, uint16_t ytile, uint8_t z);

private:
    bool load_objblk(Configuration& config);
    void load_superchunk(std::istream& is, std::list<Obj>& objlist);
    bool load_objlist(Configuration& config);
    void draw_actors(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z, bool toptile);

private:
    TileManager*    m_tile_manager;

    std::list<Obj>  m_surface_objs[8][8];
    std::list<Obj>  m_dungeon_objs[5];
    Actor           m_actors[256];
};