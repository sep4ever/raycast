
#include <raylib.h>
#include "raymath.h"
#include <cmath>
#include <vector>
#include <algorithm>

#include <iostream>

#include "Settings.h"
#include "Player.h"
#include "World.h"

#include "tiny_gltf_v3.h"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>

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

/*struct Triangle {
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
};*/

struct Box
{
    Vector3 center;
    Vector3 halfSize;
};

enum Mode {
    NONE,
    CIRCLE,
    BOX,
    TRIANGLE
};

struct Shape {
    Mode type;
    Vector3 pos;
    float size;
};

Mode uiMode = NONE;

int inputStep = 0; // 0=A, 1=B, 2=C
int boxStep = 0;
Vector3 tempA, tempB, tempC;
Vector3 boxTempCenter;
Vector3 boxTempHalf;

string input;
Rectangle btnCircle   = {10, 200, 140, 30};
Rectangle btnBox   = {10, 240, 140, 30};
Rectangle btnTriangle = {10, 280, 140, 30};

void DrawUI(float camPosArr[3], int locTriCount, int locTriA, int locTriA0);

struct Triangle
{
    float A[4];
    float B[4];
    float C[4];

    Triangle(Vector3 a, Vector3 b, Vector3 c)
    {
        A[0]=a.x; A[1]=a.y; A[2]=a.z; A[3]=0;
        B[0]=b.x; B[1]=b.y; B[2]=b.z; B[3]=0;
        C[0]=c.x; C[1]=c.y; C[2]=c.z; C[3]=0;
    }
};

// парсит "1/2/3" → 1
static int ParseIndex(const string& s)
{
    return stoi(s.substr(0, s.find('/'))) - 1;
}

// превращает face (n-gon) в треугольники
static void FanTriangulate(
    const vector<Vector3>& v,
    const vector<int>& face,
    vector<Triangle>& out)
{
    for (int i = 1; i < face.size() - 1; i++)
    {
        out.emplace_back(
            v[face[0]],
            v[face[i]],
            v[face[i + 1]]
        );
    }
}

vector<Triangle> LoadOBJ(const string& path, float scale = 1.0f)
{
    vector<Vector3> vertices;
    vector<Triangle> triangles;

    ifstream file(path);
    if (!file.is_open())
    {
        cout << "Failed to open OBJ: " << path << endl;
        return triangles;
    }

    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        string type;
        ss >> type;

        // вершины
        if (type == "v")
        {
            Vector3 p;
            ss >> p.x >> p.y >> p.z;

            p.x *= scale;
            p.y *= scale;
            p.z *= scale;

            vertices.push_back(p);
        }

        // faces
        else if (type == "f")
        {
            vector<int> face;
            string token;

            while (ss >> token)
            {
                face.push_back(ParseIndex(token));
            }

            if (face.size() < 3)
                continue;

            FanTriangulate(vertices, face, triangles);
        }
    }

    return triangles;
}

int main(void)
{
    // Инициализация игрока (z = 0)
    player.position = { 0.0f, 0.0f, 0.0f };

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

    bool useRayTrace = true; // переключатель режима (F1)
    bool pause = false;

    vector<Triangle> triangles;// = LoadOBJ("Otb/Telegraph(OOR).obj", 100);
    vector<float> triA, triB, triC;
    auto addModel = [&](const string& path, float size)
    {
        auto m = LoadOBJ(path, size);
        triangles.insert(triangles.end(), m.begin(), m.end());
    };

    addModel("Otb/Otb.obj", 100);
    //addModel("Otb/Telegraph(OOR).obj", 100);
    
    vector<float> spheresData;
    vector<Box> boxes;

    //triangles.push_back(triangle);

    bool mouseOnText = false;

    //string name;
    int letterCount = 0;

    while (!WindowShouldClose())
    {
        // Обновления игрока
        if (!pause)
        {
            player.MoveRayCast();
            player.RotateRayCast();
            HideCursor();
        }
        else
            ShowCursor();
        if (IsKeyPressed(KEY_F1)) useRayTrace = !useRayTrace;
        if (IsKeyPressed(KEY_P)) pause = !pause;

        Vector2 mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            if (CheckCollisionPointRec(mouse, btnTriangle))
            {
                uiMode = TRIANGLE;
                inputStep = 0;
                input.clear();
            }
            if (CheckCollisionPointRec(mouse, btnCircle))
            {
                uiMode = CIRCLE;
                inputStep = 0;
                input.clear();
            }
            if (CheckCollisionPointRec(mouse, btnBox))
            {
                uiMode = BOX;
                inputStep = 0;
                input.clear();
            }
        }

        int key = GetCharPressed();
        while (key > 0)
        {
            if ((key >= '0' && key <= '9') || key == '-' || key == '.' || key == ',' || key == ';')
                input.push_back((char)key);

            key = GetCharPressed();
        }

        if (IsKeyPressedRepeat(KEY_BACKSPACE))
        {
            if (!input.empty())
                input.pop_back();
        }

        if (uiMode == BOX && IsKeyPressed(KEY_ENTER))
        {
            float x,y,z;

            for (char& c : input)
                if (c == ',') c = '.';

            if (boxStep == 0)
            {
                sscanf(input.c_str(), "%f;%f;%f", &x,&y,&z);
                boxTempCenter = {x,y,z};
            }
            else
            {
                sscanf(input.c_str(), "%f;%f;%f", &x,&y,&z);
                boxTempHalf = {x,y,z};
            }

            input.clear();
            boxStep++;

            if (boxStep == 2)
            {
                boxes.push_back({boxTempCenter, boxTempHalf});
                boxStep = 0;
                uiMode = NONE;
            }
        }

        if (uiMode == CIRCLE && IsKeyPressed(KEY_ENTER))
        {
            float x, y, z, size;

            for (char& c : input)
                if (c == ',') c = '.';

            sscanf(input.c_str(), "%f;%f;%f;%f", &x, &y, &z, &size);
            input.clear();
            spheresData.push_back(x); spheresData.push_back(y);  spheresData.push_back(z); spheresData.push_back(size);
            uiMode = NONE;
        }

        if (uiMode == TRIANGLE && IsKeyPressed(KEY_ENTER))
        {
            float x, y, z;

            for (char& c : input)
                if (c == ',') c = '.';

            sscanf(input.c_str(), "%f;%f;%f", &x, &y, &z);

            if (inputStep == 0) tempA = {x, y, z};
            if (inputStep == 1) tempB = {x, y, z};
            if (inputStep == 2) tempC = {x, y, z};

            input.clear();
            inputStep++;

            if (inputStep == 3)
            {
                Triangle t(
                    tempA,
                    tempB,
                    tempC
                );

                triangles.push_back(t);

                uiMode = NONE;
                inputStep = 0;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (useRayTrace && shaderLoaded)
        {

			//if (IsKeyPressed(KEY_C)) triangles.clear();

            // Униформы общего назначения
            float resolution[2] = { (float)sets.screenWidth, 
                                    (float)sets.screenHeight };
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
            int sphereCount = spheresData.size() / 4;

            SetShaderValue(rtShader, locSphereCount, &sphereCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locSpheres, spheresData.data(), SHADER_UNIFORM_VEC4, sphereCount);

            triA.clear();
            triB.clear();
            triC.clear();
            triA.reserve(triangles.size() * 4);
            triB.reserve(triangles.size() * 4);
            triC.reserve(triangles.size() * 4);

            for (auto& t : triangles)
            {
                triA.insert(triA.end(), {t.A[0], t.A[1], t.A[2], t.A[3]});
                triB.insert(triB.end(), {t.B[0], t.B[1], t.B[2], t.B[3]});
                triC.insert(triC.end(), {t.C[0], t.C[1], t.C[2], t.C[3]});
            }

            int triCount = triangles.size();
            SetShaderValue(rtShader, locTriCount, &triCount, SHADER_UNIFORM_INT);

            SetShaderValueV(rtShader, locTriA, triA.data(), SHADER_UNIFORM_VEC4, triCount);
            SetShaderValueV(rtShader, locTriB, triB.data(), SHADER_UNIFORM_VEC4, triCount);
            SetShaderValueV(rtShader, locTriC, triC.data(), SHADER_UNIFORM_VEC4, triCount);

            // Boxes (AABB): центр (x, y, z)
            int boxCount = boxes.size();

            vector<float> boxCenterData;
            vector<float> boxHalfData;

            for (auto& b : boxes)
            {
                boxCenterData.push_back(b.center.x);
                boxCenterData.push_back(b.center.y);
                boxCenterData.push_back(b.center.z);
                boxCenterData.push_back(0.0f);

                boxHalfData.push_back(b.halfSize.x);
                boxHalfData.push_back(b.halfSize.y);
                boxHalfData.push_back(b.halfSize.z);
                boxHalfData.push_back(0.0f);
            }

            SetShaderValue(rtShader, locBoxCount, &boxCount, SHADER_UNIFORM_INT);
            SetShaderValueV(rtShader, locBoxCenter, boxCenterData.data(), SHADER_UNIFORM_VEC4, boxCount);
            SetShaderValueV(rtShader, locBoxHalf, boxHalfData.data(), SHADER_UNIFORM_VEC4, boxCount);

            // Рисуем с шейдером
            BeginShaderMode(rtShader);
            DrawRectangle(0, 0, sets.screenWidth, sets.screenHeight, WHITE);
            EndShaderMode();

            DrawUI(camPosArr, locTriCount, locTriA, locTriA0);
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

void DrawUI(float camPosArr[3], int locTriCount, int locTriA, int locTriA0)
{
    // Диагностика на экране
    DrawText(TextFormat("locTriCount=%d locTriA=%d locTriA0=%d", locTriCount, locTriA, locTriA0), 10, 28, 12, RED);
    DrawText(TextFormat("cam=(%.1f,%.1f,%.1f)", camPosArr[0], camPosArr[1], camPosArr[2]), 10, 44, 12, RED);

    DrawText(TextFormat("Input: %s", input.c_str()), 10, 320, 20, BLACK);
    if (uiMode == TRIANGLE)
    {
        DrawText("Enter: x;y;z for each vertex", 10, 350, 15, GRAY);
        DrawText(TextFormat("Step: %d/3", inputStep), 10, 370, 15, GRAY);
    }
    if (uiMode == CIRCLE)
    {
        DrawText("Enter: x;y;z;size", 10, 350, 15, GRAY);
    }
    if (uiMode == BOX)
    {
        if (boxStep == 0) DrawText("Enter: centerX;centerY;centerZ", 10, 350, 15, GRAY);
        if (boxStep == 1) DrawText("Enter: halfX;halfY;halfZ", 10, 350, 15, GRAY);
    }

    DrawRectangleRec(btnCircle, uiMode == CIRCLE ? RED : DARKGRAY);
    DrawRectangleRec(btnTriangle, uiMode == TRIANGLE ? RED : DARKGRAY);
    DrawRectangleRec(btnBox, uiMode == BOX ? RED : DARKGRAY);
    DrawText("Triangle", btnTriangle.x + 10, btnTriangle.y + 10, 10, WHITE);
    DrawText("Circle", btnCircle.x + 10, btnCircle.y + 10, 10, WHITE);
    DrawText("Box", btnBox.x + 10, btnBox.y + 10, 10, WHITE);
}