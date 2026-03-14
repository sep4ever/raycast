#include <raylib.h>
#include "raymath.h"
#include <cmath>
#include <vector>
#include <algorithm>

#include "Settings.h"
#include "Player.h"
#include "World.h"

using namespace std;

// Иниты.
Settings sets;
Player player(&sets);
World world(&player);


int main(void)
{
	player.position = { 160, 160 };


    InitWindow(sets.screenWidth, sets.screenHeight, "Raylib Raycast");

	SetTargetFPS(60);
	HideCursor();

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        player.MoveRayCast();
        player.RotateRayCast();

        world.Render(player.rayCount, player.hitWall, player.distance, player.UpdateRays());
		world.DrawMinimap();

        DrawFPS(10, 90);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}