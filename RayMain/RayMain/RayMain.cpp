#include <raylib.h>
#include <cmath>
#include <vector>

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

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

		player.Move();
        player.Rotate();

        world.Render(player.rayCount, player.hitWall, player.distance, player.UpdateRays());
		DrawText(TextFormat("%.2f", player.yAngle), 10, 10, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}