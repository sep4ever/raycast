
#include "raylib.h"

#include "Player.h"
#include <cmath>

#include "algorithm"

using namespace std;

Player::Player(Settings* s) : sets(s) {}

float Player::UpdateRays()
{
    //rays.resize(rayCount);

    float projPlaneDist = (sets->screenWidth / 2.0f) / tanf(fov / 2.0f);

    return projPlaneDist;
}

void Player::MoveRayCast()
{
	float dt = GetFrameTime();
    int TILE = sets->TILE_SIZE;
    auto& map = sets->map;

    float dx = cosf(radAngle) * speed * dt;
    float dy = sinf(radAngle) * speed * dt;  // ИСПРАВЛЕНО: было -sinf (инвертировало Y)

    // Стрейф (боком)
    float sx = sinf(radAngle) * speed * dt;
    float sy = -cosf(radAngle) * speed * dt; // ИСПРАВЛЕНО: было наоборот

    Vector3 newPos = position;

    if (IsKeyDown(KEY_W)) { newPos.x += dx; newPos.y += dy; }
    if (IsKeyDown(KEY_S)) { newPos.x -= dx; newPos.y -= dy; }
    if (IsKeyDown(KEY_A)) { newPos.x += sx; newPos.y += sy; }
    if (IsKeyDown(KEY_D)) { newPos.x -= sx; newPos.y -= sy; }

    if (IsKeyDown(KEY_SPACE)) {
        yOffset += 1;
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        yOffset -= 1;
    }

    int tileX = (int)(newPos.x / TILE);
    int tileY = (int)(newPos.y / TILE);

    bool inBounds = true;//tileX >= 0 && tileX < sets->mapWidth &&
        tileY >= 0 && tileY < sets->mapHeight;

    //if (inBounds && map[tileY][tileX] == 0)
    position = newPos;
}

void Player::RotateRayCast()
{
    UpdateRays();

    Vector2 mousePos = { GetMouseX(), GetMouseY() };
    float centerY = sets->screenHeight / 2.0f;
    float centerX = sets->screenWidth / 2.0f;

    /*float deltaX = mousePos.x - centerX;
    float deltaY = mousePos.y - centerY;

    radAngle += deltaX * 0.003f;
    yAngle -= deltaY;
    yAngle = clamp(yAngle, -360.0f, 360.0f);

    SetMousePosition((int)centerX, (int)centerY);*/

    float deltaX = mousePos.x - centerX;
    float deltaY = mousePos.y - centerY;

    // yaw — в радианах (как было)
    radAngle += deltaX * 0.003f;

    // pitch — интегрируем с чувствительностью и храним УЖЕ в радианах
    const float pitchSensitivity = 0.003f; // подберите по вкусу
    yRadOffset -= deltaY * pitchSensitivity;   // теперь yAngle в радианах

    // ограничим наклон (например ±70 градусов ≈ ±1.22 рад)
    const float maxPitch = 1.22f;
    yRadOffset = clamp(yRadOffset, -maxPitch, maxPitch);
    yAngle = yRadOffset * (sets->screenHeight / 2.0f);

    SetMousePosition((int)centerX, (int)centerY);
};