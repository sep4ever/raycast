#include "World.h"
#include "raylib.h"
#include "Player.h"
#include <cmath>
#include <algorithm>

using namespace std;

World::World(Player* player) : p(player) {}

void World::Render(int rayCount, bool hitWall, float distance, float projPlaneDist)
{
	int TILE_SIZE = p->sets->TILE_SIZE;
	int screenWidth = p->sets->screenWidth;
	int screenHeight = p->sets->screenHeight;
    auto& map = p->sets->map;
    for (int i = 0; i < rayCount; i++)
    {
        hitWall = false;
        distance = 0.0f;

        float t = (float)i / (rayCount - 1);
        float angle = p->radAngle - p->fov * 0.5f + t * p->fov;

        Vector3 dir = { cosf(angle), -sinf(angle), 0.0f };

        while (!hitWall && distance < 1000.0f)
        {
            distance++;

            int rayX = (p->position.x + dir.x * distance) / TILE_SIZE;
            int rayY = (p->position.y + dir.y * distance) / TILE_SIZE;

            if (rayX >= 5 || rayY >= 5 || rayX < 0 || rayY < 0)
            {
                break;
            }

            if (map[rayY][rayX] == 1)
            {
                hitWall = true;
            }

            //int displacement = p->yAngle;//(int) (TILE_SIZE * p->wallScale / (distance * cos(p->yAngle)));
            //float displacementAngle = displacement / p->rotYSpeed;
            Vector2 displacement = {p->radAngle, p->yAngle};

            float corrected = distance * cos(angle - p->radAngle);
            if (corrected < 0.0001f)
                corrected = 0.0001f;

            float wallHeight = (TILE_SIZE * p->wallScale / corrected) * projPlaneDist;

            float columnWidth = screenWidth / rayCount;

            

            int drawStart = (int)((screenHeight / 2.0f) - (wallHeight / 2.f) + displacement.y);
            int drawEnd = drawStart + (int)wallHeight;

            if (drawStart < 0)
                drawStart = 0;

            if (drawEnd > screenHeight)
                drawEnd = screenHeight;

            int x = i * columnWidth;


            float shade = 1.0f - min((distance) / 100.0f, 1.0f);
            unsigned char col = (unsigned char)(180.0f * shade) + 30;

            Color wallColor = { col, col, col, 255 };
			Color floorColor = { (unsigned char)(100.0f * shade), (unsigned char)(100.0f * shade) + 30, (unsigned char)(100.0f * shade) + 30, 255 };
            if (drawEnd < screenHeight)
            {
                DrawRectangle(x, drawEnd , columnWidth, screenHeight - drawEnd, floorColor);
            }

            DrawRectangle(
                x,
                drawStart,
                columnWidth,
                wallHeight,
                wallColor
            );
        }
    }
}