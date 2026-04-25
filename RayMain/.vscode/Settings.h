#ifndef SETTINGS_H
#define SETTINGS_H

#include <array>

using namespace std;

struct Settings
{
    // Размер карты.

	static constexpr int mapWidth = 5;
	static constexpr int mapHeight = 5;

    array<array<int, mapWidth>, mapHeight> map = {{ // Карта мира. 1 - стена, 0 - пустое пространство.
    {{0,0,0,0,0}},
    {{0,0,0,0,0}},
    {{0,0,0,0,0}},
    {{0,0,0,0,0}},
    {{0,0,0,0,0}}
    }};
    const int TILE_SIZE = 64;

    // Размеры окна.
    int screenHeight = 920;
    int screenWidth = 1280;
};

#endif
