
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

static void DrawDebugText(const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    DrawText(buf, 10, 10, 12, BLACK);
}

int main(void)
{
    // Инициализация игрока (z = 0)
    player.position = { 160.0f, 160.0f, 0.0f };

    InitWindow(sets.screenWidth, sets.screenHeight, "Raylib Raycast");
    SetTargetFPS(60);
    HideCursor();

    // Загружаем шейдер после InitWindow
    Shader rtShader = { 0 };
    bool shaderLoaded = false;
    if (FileExists("shader.frag"))
    {
        rtShader = LoadShader(0, "shader.frag");
        shaderLoaded = (rtShader.id != 0);
    }

    // Если шейдер загружен — получаем локации uniform'ов
    int locResolution = -1, locTime = -1, locCamPos = -1, locCamYaw = -1, locCamPitch = -1;
    int locSphereCount = -1, locSpheres = -1;
    int locTriCount = -1, locTriA = -1, locTriB = -1, locTriC = -1;
    int locBoxCount = -1, locBoxCenter = -1, locBoxHalf = -1;
    int locTriA0 = -1, locTriB0 = -1, locTriC0 = -1;

    if (shaderLoaded)
    {
        locResolution = GetShaderLocation(rtShader, "u_resolution");
        locTime = GetShaderLocation(rtShader, "u_time");
        locCamPos = GetShaderLocation(rtShader, "u_camPos");
        locCamYaw = GetShaderLocation(rtShader, "u_camYaw");
        locCamPitch = GetShaderLocation(rtShader, "u_camPitch");

        locSphereCount = GetShaderLocation(rtShader, "u_sphereCount");
        locSpheres = GetShaderLocation(rtShader, "u_spheres");

        locTriCount = GetShaderLocation(rtShader, "u_triCount");
        locTriA = GetShaderLocation(rtShader, "u_triA");
        locTriB = GetShaderLocation(rtShader, "u_triB");
        locTriC = GetShaderLocation(rtShader, "u_triC");

        locBoxCount = GetShaderLocation(rtShader, "u_boxCount");
        locBoxCenter = GetShaderLocation(rtShader, "u_boxCenters");
        locBoxHalf = GetShaderLocation(rtShader, "u_boxHalfSizes");

        // Попробуем получить локации первого элемента массива (если драйвер/GL поддерживает)
        locTriA0 = GetShaderLocation(rtShader, "u_triA[0]");
        locTriB0 = GetShaderLocation(rtShader, "u_triB[0]");
        locTriC0 = GetShaderLocation(rtShader, "u_triC[0]");
    }

    bool useRayTrace = false; // переключатель режима (F1)

    while (!WindowShouldClose())
    {
        // Обновления игрока
        player.MoveRayCast();
        player.RotateRayCast();

        if (IsKeyPressed(KEY_F1)) useRayTrace = !useRayTrace;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (useRayTrace && shaderLoaded)
        {
            // Униформы общего назначения
            float resolution[2] = { (float)sets.screenWidth, (float)sets.screenHeight };
            float t = GetTime();
            // камера: x и "z в шейдере" берем из player.position.x и player.position.y
            float camPosArr[3] = { player.position.x, player.yOffset, player.position.y };
            float yaw = player.radAngle;
            float pitch = player.yRadOffset;

            SetShaderValue(rtShader, locResolution, resolution, SHADER_UNIFORM_VEC2);
            SetShaderValue(rtShader, locTime, &t, SHADER_UNIFORM_FLOAT);
            SetShaderValue(rtShader, locCamPos, camPosArr, SHADER_UNIFORM_VEC3);
            SetShaderValue(rtShader, locCamYaw, &yaw, SHADER_UNIFORM_FLOAT);
            SetShaderValue(rtShader, locCamPitch, &pitch, SHADER_UNIFORM_FLOAT);

            // --- Формируем сцену: сферы, треугольники, коробки ---
            // Используем абсолютные мировые координаты для объектов (не привязывать к камере)
            int sphereCount = 3;
            std::vector<float> spheresData;
            spheresData.reserve(sphereCount * 4);
            // x, y(height), z(depth), radius
            spheresData.push_back(200.0f); spheresData.push_back(20.0f);  spheresData.push_back(-300.0f); spheresData.push_back(80.0f);
            spheresData.push_back(0.0f);   spheresData.push_back(40.0f);  spheresData.push_back(-400.0f); spheresData.push_back(50.0f);
            spheresData.push_back(-150.0f); spheresData.push_back(30.0f);  spheresData.push_back(-250.0f); spheresData.push_back(40.0f);

            SetShaderValue(rtShader, locSphereCount, &sphereCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locSpheres, spheresData.data(), SHADER_UNIFORM_VEC4, sphereCount);

            // Треугольники: vec4 per vertex (z = player.position.y ...)
            int triCount = 1;
            float triA[4] = { 100.0f, 20.0f, -200.0f, 0.0f };
            float triB[4] = { 200.0f, 20.0f, -250.0f, 0.0f };
            float triC[4] = { 300.0f, 10.0f, -300.0f, 0.0f };

            SetShaderValue(rtShader, locTriCount, &triCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locTriA, triA, SHADER_UNIFORM_VEC4, triCount);
            SetShaderValueV(rtShader, locTriB, triB, SHADER_UNIFORM_VEC4, triCount);
            SetShaderValueV(rtShader, locTriC, triC, SHADER_UNIFORM_VEC4, triCount);

            // Boxes (AABB): центр (x, y, z) — z = player.position.y
            int boxCount = 1;
            float boxCenter[4] = { 0.0f, 32.0f, -400.0f, 0.0f };
            float boxHalf[4] = { 50.0f, 32.0f, 50.0f, 0.0f };
            SetShaderValue(rtShader, locBoxCount, &boxCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locBoxCenter, boxCenter, SHADER_UNIFORM_VEC4, boxCount);
            SetShaderValueV(rtShader, locBoxHalf, boxHalf, SHADER_UNIFORM_VEC4, boxCount);

            // Рисуем с шейдером
            BeginShaderMode(rtShader);
            DrawRectangle(0, 0, sets.screenWidth, sets.screenHeight, WHITE);
            EndShaderMode();

            // Диагностика на экране
            DrawText(TextFormat("locTriCount=%d locTriA=%d locTriA0=%d", locTriCount, locTriA, locTriA0), 10, 28, 12, RED);
            DrawText(TextFormat("triCount=%d   cam=(%.1f,%.1f,%.1f)", triCount, camPosArr[0], camPosArr[1], camPosArr[2]), 10, 44, 12, RED);
        }
        else
        {
            // fallback: штатный рендер (raycast)
            world.Render(player.rayCount, player.hitWall, player.distance, player.UpdateRays());
        }

        DrawFPS(10, 90);
        EndDrawing();
    }

    if (shaderLoaded) UnloadShader(rtShader);
    CloseWindow();

    return 0;
}