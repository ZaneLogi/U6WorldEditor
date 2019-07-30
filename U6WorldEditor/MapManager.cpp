#include "stdafx.h"
#include "MapManager.h"

bool MapManager::init(Configuration& config)
{
    color_palette.init(config);
    tile_image.init(config);
    tile_manager.init(config, &tile_image);
    basemap.init(config, &tile_manager);
    obj_manager.init(config, &tile_manager);
    return true;
}

void MapManager::update(DibSection& ds)
{
    color_palette.update(ds);
    tile_manager.update();
}

void MapManager::draw(DibSection& ds, uint16_t world_tile_x, uint16_t world_tile_y, uint8_t z)
{
    basemap.draw(ds, world_tile_x, world_tile_y, z);
    obj_manager.draw(ds, world_tile_x, world_tile_y, z, false);
    obj_manager.draw(ds, world_tile_x, world_tile_y, z, true);
}