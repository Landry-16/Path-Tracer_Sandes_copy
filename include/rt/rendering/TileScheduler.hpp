/*
** EPITECH PROJECT, 2026
** clean_raytracer
** File description:
** TileScheduler
*/

#ifndef RT_TILESCHEDULER_HPP
    #define RT_TILESCHEDULER_HPP

    #include <vector>
    #include <atomic>

struct Tile {
    int startX;
    int startY;
    int endX;
    int endY;
};

class TileScheduler {
public:
    TileScheduler(int imageWidth, int imageHeight, int tileSize);
    
    bool getNextTile(Tile &tile);
    int totalTiles() const;
    int completedTiles() const;
    
private:
    std::vector<Tile> tiles;
    std::atomic<int> nextTileIndex;
    std::atomic<int> completedCount;
};

#endif // RT_TILESCHEDULER_HPP
