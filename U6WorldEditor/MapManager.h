#pragma once

#include "DibSection.h"
#include "Configuration.h"
#include "ColorPalette.h"
#include "TileImage.h"
#include "TileManager.h"
#include "BaseMap.h"
#include "ObjManager.h"
#include "Text.h"

class MapManager
{
public:
    bool init(Configuration& config);
    void update(DibSection& ds);
    void draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z);


public:
    ColorPalette    color_palette;
    TileImage       tile_image;
    TileManager     tile_manager;
    BaseMap         basemap;
    ObjManager      obj_manager;
    Text            text;
};