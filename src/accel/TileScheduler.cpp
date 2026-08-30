/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** TileScheduler
*/

#include "rt/rendering/TileScheduler.hpp"

TileScheduler::TileScheduler(int imageWidth, int imageHeight, int tileSize)
    : nextTileIndex(0), completedCount(0)
{    
    for (int y = 0; y < imageHeight; y += tileSize) {
        for (int x = 0; x < imageWidth; x += tileSize) {
            Tile tile;
            tile.startX = x;
            tile.startY = y;
            tile.endX = (x + tileSize < imageWidth) ? x + tileSize : imageWidth;
            tile.endY = (y + tileSize < imageHeight) ? y + tileSize : imageHeight;
            tiles.push_back(tile);
        }
    }
}

bool TileScheduler::getNextTile(Tile &tile)
{
    int index = nextTileIndex.fetch_add(1);
    
    if (index >= static_cast<int>(tiles.size())) {
        return false;
    }
    
    tile = tiles[index];
    return true;
}

int TileScheduler::totalTiles() const
{
    return static_cast<int>(tiles.size());
}

int TileScheduler::completedTiles() const
{
    return completedCount.load();
}
