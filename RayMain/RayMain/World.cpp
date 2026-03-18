#include "World.h"
#include "raylib.h"
#include "Player.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include <iostream>

using namespace std;

World::World(Player* player) : p(player) {}
static float IntersectCircle2D(float ox, float oy, float dx, float dy,
    float cx, float cy, float r)
{
    float lx = ox - cx, ly = oy - cy;
    float a = dx * dx + dy * dy;
    float b = 2 * (lx * dx + ly * dy);
    float c = lx * lx + ly * ly - r * r;
    float D = b * b - 4 * a * c;
    if (D < 0) return -1.f;
    float t = (-b - sqrtf(D)) / (2 * a);
    return t > 0 ? t : -1.f;
}


float World::SphereIntersect(Vector3 ro, Vector3 rd, Vector3 center, float radius)
{
    Vector3 L = { ro.x - center.x, ro.y - center.y, ro.z - center.z };
    float b = L.x * rd.x + L.y * rd.y + L.z * rd.z;
    float c = L.x * L.x + L.y * L.y + L.z * L.z - radius * radius;
    float h = b * b - c;
    if (h < 0) return -1.0f;
    h = sqrt(h);
    return -b - h; // ближайшая точка пересечения вдоль луча
}

void World::DrawFloorCeiling(float projPlaneDist)
{
    int W = p->sets->screenWidth;
    int H = p->sets->screenHeight;
    int horizon = H / 2 + (int)p->yAngle;

    // Вместо симметрии — просто рисуй потолок сверху до горизонта
    for (int y = 0; y < horizon; y++)
    {
        float rowDist = projPlaneDist / (float)(horizon - y);  // расстояние от горизонта вверх
        float shade = 1.f - std::min(rowDist / 4.4f, 1.f);
        unsigned char c = (unsigned char)(40.f * shade);
        DrawRectangle(0, y, W, 1, { c, c, c, 255 });
    }

    for (int y = horizon + 1; y < H; y++)
    {
        float rowDist = projPlaneDist / (float)(y - horizon);
        float shade = 1.f - min(rowDist / 4.4f, 1.f);

        // Пол
        unsigned char col = (unsigned char)(130.f * shade);
        DrawRectangle(0, y, W, 1, Color { col, col, col, 255 });
    }

    
}

void World::RayTraceScene(int width, int height)
{
    // простая сцена: одна сфера
    Vector3 spherePos = { p->position.x + 200.0f, 0.0f, p->position.y - 300.0f }; // пример положения
    float radius = 80.0f;

    // камера (позиция) — используем игрока: x,z берём из позиции игрока, y=20 (высота камеры)
    Vector3 ro = { p->position.x, 20.0f, p->position.y };

    float aspect = (float)width / (float)height;
    float scale = tanf(p->fov * 0.5f); // полукут обзора

    // простой directional light
    Vector3 lightDir = Vector3Normalize({ -0.5f, -1.0f, -0.3f });

    // Для каждого пикселя — вычисляем первичный луч и тестируем пересечение со сферой
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // NDC -> экранные координаты (-1..1)
            float ndcX = ((x + 0.5f) / (float)width) * 2.0f - 1.0f;
            float ndcY = 1.0f - ((y + 0.5f) / (float)height) * 2.0f;

            // камера пространство
            Vector3 dir = {
                ndcX * aspect * scale,
                ndcY * scale,
                -1.0f
            };

            // нормализация
            dir = Vector3Normalize(dir);

            // поворот вокруг вертикальной оси по yaw игрока (radAngle)
            float yaw = p->radAngle;
            Vector3 rd = {
                cosf(yaw) * dir.x - sinf(yaw) * dir.z,
                dir.y,
                sinf(yaw) * dir.x + cosf(yaw) * dir.z
            };
            rd = Vector3Normalize(rd);

            float t = SphereIntersect(ro, rd, spherePos, radius);
            Color pixel = BLACK;

            if (t > 0.0f)
            {
                // точка попадания
                Vector3 hitPos = { ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t };
                Vector3 normal = Vector3Normalize(Vector3Subtract(hitPos, spherePos));
                float diff = Vector3DotProduct(normal, Vector3Normalize(Vector3Negate(lightDir)));
                if (diff < 0.0f) diff = 0.0f;

                // простой цвет сферы + ламбертовское освещение
                unsigned char r = (unsigned char)clamp(diff * 200.0f + 30.0f, 0.0f, 255.0f);
                unsigned char g = (unsigned char)clamp(diff * 120.0f + 30.0f, 0.0f, 255.0f);
                unsigned char b = (unsigned char)clamp(diff * 60.0f + 30.0f, 0.0f, 255.0f);

                pixel = { r, g, b, 255 };
            }

            // Рисуем пиксель — DrawPixel быстрый для демонстрации (может быть медленно)
            DrawPixel(x, y, pixel);
        }
    }
}

void World::Render(int rayCount, bool hitWall, float distance, float projPlaneDist)
{
	int TILE_SIZE = p->sets->TILE_SIZE;
	int screenWidth = p->sets->screenWidth;
	int screenHeight = p->sets->screenHeight;
    auto& map = p->sets->map;
    
	DrawFloorCeiling(projPlaneDist);

    float objX = 2.5f * TILE_SIZE;
    float objY = 2.5f * TILE_SIZE;
    float objR = 0;//TILE_SIZE * 0.35f;  

    for (int i = 0; i < rayCount; i++)
    {
        hitWall = false;
        distance = 0.0f;

        float t = (float)i / (rayCount - 1);
        float angle = p->radAngle - p->fov * 0.5f + t * p->fov;

        float cosA = cosf(angle);
        float sinA = sinf(angle);

        float circDist = IntersectCircle2D(
            p->position.x, p->position.y,
            cosA, sinA,
            objX, objY, objR
        );
        bool hitCircle = circDist > 0 && circDist < 1000.f;

        float tileDist = 1000.f;
        while (distance < 1000.0f)
        {
            distance++;
            int rayX = (int)((p->position.x + cosA * distance) / TILE_SIZE);
            int rayY = (int)((p->position.y + sinA * distance) / TILE_SIZE);

            if (rayX < 0 || rayX >= p->sets->mapWidth ||
                rayY < 0 || rayY >= p->sets->mapHeight)
                break;

            if (map[rayY][rayX] == 1 && !hitWall)
            {
                hitWall = true;
                tileDist = distance;
                break;
            }
        }

        bool   drawCircle = hitCircle && (!hitWall || circDist < tileDist);
        float  finalDist = drawCircle ? circDist : tileDist;

        float corrected = finalDist * cosf(angle - p->radAngle);
        if (corrected < 0.0001f) corrected = 0.0001f;

        float wallHeight = (TILE_SIZE * p->wallScale / corrected) * projPlaneDist;

        int x = (int)((float)i / rayCount * screenWidth);
        int columnWidth = (int)((float)(i + 1) / rayCount * screenWidth) - x;

        // Вертикальный сдвиг взгляда (yAngle из мыши)
        float vertOffset = p->yAngle;
        int   drawStart = (int)((screenHeight / 2.f) - (wallHeight / 2.f) + vertOffset);
        int   drawEnd = drawStart + (int)wallHeight;

        if (drawStart < 0)
            drawStart = 0;

        if (drawEnd > screenHeight)
            drawEnd = screenHeight;


        float shade = 1.0f - min((distance) / 100.0f, 1.0f);
        unsigned char col = (unsigned char)(180.0f * shade);

        int clampedStart = max(drawStart, 0);

        //DrawRectangle(x, 0, columnWidth, clampedStart, BLACK);

        Color wallColor = { col, col, col, 255 };
        Color floorColor = DARKGRAY;
        if (drawEnd < screenHeight)
        {
        //    DrawRectangle(x, drawEnd, columnWidth, screenHeight - drawEnd, floorColor);
        }

        //DrawRectangle(x, drawStart, columnWidth, drawEnd - drawStart, wallColor);

        if (drawCircle) {
            // Круглый объект — синеватый цвет
            //float shade = 1.f - min(circDist / 100.f, 1.f);
            unsigned char b = (unsigned char)(200.f * shade);
            unsigned char r = (unsigned char)(80.f * shade);
            wallColor = { r, r, b, 255 };
        }
        else 
        {
            // Обычная стена — серый с затуханием
            //float shade = 1.f - min(tileDist / 100.f, 1.f);
            unsigned char c = (unsigned char)(80.f * shade);
            wallColor = { c, c, c, 255 };
        }
        int clampedEnd = min(drawEnd, screenHeight);

        DrawRectangle(x, clampedStart, columnWidth, clampedEnd - clampedStart, wallColor);

        //if (clampedEnd < screenHeight)
          //  DrawRectangle(x, clampedEnd, columnWidth, screenHeight - clampedEnd, DARKGRAY);
    }
}
void World::DrawMinimap()
{
    int   TILE = p->sets->TILE_SIZE / 5;  // уменьшаем в 5 раз
    int   offX = 10, offY = 10;
    auto& map = p->sets->map;

    for (int y = 0; y < p->sets->mapHeight; y++) {
        for (int x = 0; x < p->sets->mapWidth; x++) {
            Color c = map[y][x] == 1 ? GRAY : DARKGRAY;
            DrawRectangle(offX + x * TILE, offY + y * TILE, TILE - 1, TILE - 1, c);
        }
    }
    // Позиция игрока
    int px = offX + (int)(p->position.x / p->sets->TILE_SIZE * TILE);
    int py = offY + (int)(p->position.y / p->sets->TILE_SIZE * TILE);
    DrawRectangle(px - 2, py - 2, 4, 4, RED);
}