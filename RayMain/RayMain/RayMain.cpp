
#include <raylib.h>
#include "raymath.h"
#include <cmath>
#include <vector>
#include <algorithm>

#include <iostream>

#include "Settings.h"
#include "Player.h"
#include "World.h"

#include <string>
#include <sstream>

using namespace std;

int maxChars = 28;

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

struct Triangle {
    float A[4];
    float B[4];
    float C[4];

    void SetVertices(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
    {
        A[0] = ax; A[1] = ay; A[2] = az; A[3] = 0.0f;
        B[0] = bx; B[1] = by; B[2] = bz; B[3] = 0.0f;
        C[0] = cx; C[1] = cy; C[2] = cz; C[3] = 0.0f;
	}

    Triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
    {
		SetVertices(ax, ay, az, bx, by, bz, cx, cy, cz);
    }
};

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

    vector<Triangle> triangles;
    
    //triangles.push_back(triangle);

    bool mouseOnText = false;

    string name;
    int letterCount = 0;

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
            if (IsKeyPressed(KEY_SLASH))
            {
                mouseOnText = !mouseOnText;
			}
            if (mouseOnText)
            {
                SetMouseCursor(MOUSE_CURSOR_IBEAM);

                int key = GetCharPressed();

                while (key > 0)
                {
                    if (((key >= 45 && key <= 57 && key != 47) || key == 59))
                    {
						name.push_back((char)key);
                        //name[letterCount] = (float)key;
                        //name[letterCount + 1] = '\0';
                        letterCount++;
                    }

                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) && !name.empty())
                {
                    name.pop_back();
                    letterCount--;
                }
            }
            else 
            {
                if (letterCount > 0) 
                {
                    float values[9] = { 0.0f }; // по умолчанию нули

                    if (name.data() != nullptr)
                    {
                        string str = name;
                        string temp;
                        stringstream ss(str);

                        int i = 0;

                        while (getline(ss, temp, ';') && i < 9)
                        {
                            try
                            {
                                values[i] = stof(temp);
                            }
                            catch (...)
                            {
                                values[i] = 0.0f; // если ошибка — ноль
                            }
                            i++;
                        }
                    }

                    Triangle triangle(
                        player.position.x + values[0],
                        player.position.y + values[1],
                        player.position.z + values[2],

                        player.position.x + values[3],
                        player.position.y + values[4],
                        player.position.z + values[5],

                        player.position.x + values[6],
                        player.position.y + values[7],
                        player.position.z + values[8]
                    );
                    triangles.push_back(triangle);
					for (int i = 0; i < 9; i++) values[i] = 0.0f;
                    name.clear();
                    letterCount = 0;
                }
                SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            }

			if (IsKeyPressed(KEY_C)) triangles.clear();

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
            vector<float> spheresData;
            spheresData.reserve(sphereCount * 4);
            // x, y(height), z(depth), radius
            spheresData.push_back(-200.0f); spheresData.push_back(20.0f);  spheresData.push_back(-300.0f); spheresData.push_back(80.0f);
            spheresData.push_back(0.0f);   spheresData.push_back(40.0f);  spheresData.push_back(-400.0f); spheresData.push_back(50.0f);
            spheresData.push_back(-150.0f); spheresData.push_back(30.0f);  spheresData.push_back(-250.0f); spheresData.push_back(40.0f);

            SetShaderValue(rtShader, locSphereCount, &sphereCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locSpheres, spheresData.data(), SHADER_UNIFORM_VEC4, sphereCount);

            // Треугольники: vec4 per vertex (z = player.position.y ...)
            int triCount = triangles.size();

            SetShaderValue(rtShader, locTriCount, &triCount, SHADER_UNIFORM_INT);

            for (int i = 0; i < triangles.size(); i++) 
            {
                SetShaderValueV(rtShader, locTriA, triangles[i].A, SHADER_UNIFORM_VEC4, triCount);
                SetShaderValueV(rtShader, locTriB, triangles[i].B, SHADER_UNIFORM_VEC4, triCount);
                SetShaderValueV(rtShader, locTriC, triangles[i].C, SHADER_UNIFORM_VEC4, triCount);
            }

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
            
            string text = "Triangle coordinates(example: (x;y;z)): " + name;

            DrawText(text.c_str(), 5, 68, 20, MAROON);
            DrawText("(Write coordinates for each verticy!)", 5, 88, 20, RED);
        }
        else
        {
            // fallback: штатный рендер (raycast)
            world.Render(player.rayCount, player.hitWall, player.distance, player.UpdateRays());
        }

        DrawFPS(GetScreenWidth() - 90, 0);
        EndDrawing();
    }

    if (shaderLoaded) UnloadShader(rtShader);
    CloseWindow();

    return 0;
}