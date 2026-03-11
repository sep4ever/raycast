#include <raylib.h>
#include <cmath>
#include <vector>
#include <iostream>

using namespace std;
int map[5][5] = {
    {1,1,1,1,1},
    {1,0,0,0,1},
    {1,1,0,1,1},
    {1,0,0,0,1},
    {1,1,1,1,1}
};
const int TILE_SIZE = 64;

int screenHeight = 920;
int screenWidth = 1280;

class Player 
{
public:
    float size = 40.0f;
    float radAngle = 0.0f;

    float speed = 15.0f;

    float rotSpeed = 5.0f;

    Vector2 position = {0, 0};

    std::vector<Ray> rays;
    int rayCount = 120;
    float fov = 90 * (3.14159265f / 180.0f); // 90 градусов в радианах
    float rayLength = 300.0f;

    bool hitWall = false;
    float distance = 0.0f;
    
    float wallScale = 0.5f;

    void UpdateRays()
    {
        rays.resize(rayCount);

        float projPlaneDist = (screenWidth / 2.0f) / tanf(fov / 2.0f);

        for (int i = 0; i < rayCount; i++)
        {
            hitWall = false;
            distance = 0.0f;

            float t = (float)i / (rayCount - 1);
            float angle = radAngle - fov * 0.5f + t * fov;

            Vector3 dir = { cosf(angle), -sinf(angle), 0.0f };
            rays[i].position = { position.x, position.y, 0.0f };
            rays[i].direction = dir;
            while (!hitWall && distance < 1000.0f) 
            {
                distance++;

                int rayX = (position.x + dir.x * distance) / TILE_SIZE;
                int rayY = (position.y + dir.y * distance) / TILE_SIZE;

                if (rayX >= 5 || rayY >= 5 || rayX < 0 || rayY < 0) {
                    break;

                }

                if (map[rayY][rayX] == 1)
                {
                    hitWall = true;
                }

                float corrected = distance * cos(angle - radAngle);
                if (corrected < 0.0001f) corrected = 0.0001f;

                float wallHeight = (TILE_SIZE * wallScale / corrected) * projPlaneDist;

                float columnWidth = screenWidth / rayCount;

                int drawStart = (int)( (screenWidth / 2.0f) - (wallHeight / 1.65f));
                int drawEnd = drawStart + (int)wallHeight;

                if (drawStart < 0) drawStart = 0;
                if (drawEnd > screenHeight) drawEnd = screenHeight;

                int x = i * columnWidth;

                float shade = 1.0f - std::min(distance / 100.0f, 1.0f); // регулируем диапазон
                unsigned char col = (unsigned char)(180.0f * shade) + 30; // минимальная яркость
                Color wallColor = { col, col, col, 255 };

                if (drawEnd < screenHeight) {
                    DrawRectangle(x, drawEnd, columnWidth, screenHeight - drawEnd, BROWN);
                }

                DrawRectangle(
                    i * columnWidth,
                    drawStart,
                    columnWidth,
                    wallHeight,
                    wallColor
                );
            }
            
        }
    }

    void Move()
    {
        // Расчёт смещений по направлению текущего угла
        float dx = cosf(radAngle) * speed * GetFrameTime();
        float dy = sinf(radAngle) * speed * GetFrameTime();

        if (IsKeyDown(KEY_UP)) {
            position.x += dx;
            position.y -= dy; // вычитаем, т.к. в экране положительное y — вниз
        }
        if (IsKeyDown(KEY_DOWN)) {
            position.x -= dx;
            position.y += dy;
        }
	}

    void Rotate()
    {
        UpdateRays();
        if (IsKeyDown(KEY_RIGHT)) radAngle += rotSpeed * GetFrameTime();
        if (IsKeyDown(KEY_LEFT)) radAngle -= rotSpeed * GetFrameTime();
	}
};

void DrawMap()
{
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            if (map[y][x] == 1)
            {
                DrawRectangle(
                    x * TILE_SIZE,
                    y * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE,
                    BLUE
                );
            }
        }
    }
}

int main(void)
{
    Player player;
	player.position = { 160, 160 };

    InitWindow(screenWidth, screenHeight, "Raylib Raycast");

	SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
		player.Move();
        player.Rotate();
		//DrawCircle(player.position.x, player.position.y, player.size, MAROON);
        DrawMap();
        DrawCircleV(player.position, player.size / 4.0f, MAROON);

        //DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}