#include <raylib.h>
#include <cmath>
#include <vector>
#include <iostream>
using namespace std;
int map[5][5] = {
    {1,1,1,1,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,0,0,0,1},
    {1,1,1,1,1}
};
const int TILE_SIZE = 64;

int screenHeight = 1000;
int screenWidth = 1000;

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

    void UpdateRays()
    {
        rays.resize(rayCount);

        for (int i = 0; i < rayCount; i++)
        {
            hitWall = false;
            distance = 0.0f;

            float t = (float)i / (rayCount - 1);
            float angle = radAngle - fov * 0.5f + t * fov;

            Vector3 dir = { cosf(angle), -sinf(angle), 0.0f };
            while (!hitWall && distance < 200.0f) 
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

                //DrawRay(rays[i], RED);
            }
            float corrected = distance * cos(angle - radAngle);

            float wallHeight = screenHeight / corrected;

            float columnWidth = screenHeight / rayCount;

            float drawStart = screenWidth / 2 - wallHeight / 2;

            DrawRectangle(
                i * columnWidth,
                drawStart,
                columnWidth,
                wallHeight,
                DARKGRAY
            );
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
        //DrawMap();
        //DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}