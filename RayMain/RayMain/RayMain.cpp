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

    Shader sh = LoadShader(0, "shader.frag");
    Shader rtShader = LoadShader(0, "shader.frag");
    int locResolution = GetShaderLocation(rtShader, "u_resolution");
    int locTime = GetShaderLocation(rtShader, "u_time");
    int locCamPos = GetShaderLocation(rtShader, "u_camPos");
    int locCamYaw = GetShaderLocation(rtShader, "u_camYaw");

    bool useRayTrace = false; // переключатель режима (F1)

	SetTargetFPS(60);
	HideCursor();

    while (!WindowShouldClose())
    {
        player.MoveRayCast();
        player.RotateRayCast();

        if (IsKeyPressed(KEY_F1)) useRayTrace = !useRayTrace;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (useRayTrace)
        {
            // Обновляем униформы шейдера
            float resolution[2] = { (float)sets.screenWidth, (float)sets.screenHeight };
            float t = GetTime();
            Vector3 camPos = { player.position.x, 20.0f, player.position.y }; // y = высота камеры
            float yaw = player.radAngle;

            SetShaderValue(rtShader, locResolution, resolution, SHADER_UNIFORM_VEC2);
            SetShaderValue(rtShader, locTime, &t, SHADER_UNIFORM_FLOAT);
            SetShaderValue(rtShader, locCamPos, &camPos, SHADER_UNIFORM_VEC3);
            SetShaderValue(rtShader, locCamYaw, &yaw, SHADER_UNIFORM_FLOAT);

            BeginShaderMode(rtShader);
            // Рисуем полноэкранный прямоугольник — фрагментный шейдер заполнит экран
            DrawRectangle(0, 0, sets.screenWidth, sets.screenHeight, WHITE);
            EndShaderMode();
        }
        else
        {
            // Обычный рендер (raycast)
            world.Render(player.rayCount, player.hitWall, player.distance, player.UpdateRays());
        }

        DrawFPS(10, 90);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}