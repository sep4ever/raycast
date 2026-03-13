
#include "raylib.h"

#include "Player.h"
#include <cmath>

Player::Player(Settings* s) : sets(s) {}

float Player::UpdateRays()
{
    //rays.resize(rayCount);

    float projPlaneDist = (sets->screenWidth / 2.0f) / tanf(fov / 2.0f);

    return projPlaneDist;
}

void Player::Move()
{
    float dx = cosf(radAngle) * speed * GetFrameTime();
    float dy = sinf(radAngle) * speed * GetFrameTime();

    float sideX = sinf(radAngle) * speed * GetFrameTime();
    float sideY = -cosf(radAngle) * speed * GetFrameTime();

    if (IsKeyDown(KEY_W)) {
        position.x += dx;
        position.y -= dy;
    }

    if (IsKeyDown(KEY_S)) {
        position.x -= dx;
        position.y += dy;
    }
    if (IsKeyDown(KEY_A)) {
        position.x += sideX;
        position.y -= sideY;
    }

    if (IsKeyDown(KEY_D)) {
        position.x -= sideX;
        position.y += sideY;
    }
}

void Player::Rotate()
{
    UpdateRays();

    Vector2 mousePos = { GetMouseX(), GetMouseY() };
    float centerY = sets->screenHeight / 2.0f;
    float centerX = sets->screenWidth / 2.0f;

    displacement = { centerX - mousePos.x, centerY - mousePos.y };
	radAngle -= displacement.x * 0.003f;
	if (yAngle >= -360 && yAngle <= 1200) yAngle += displacement.y;
	if (yAngle < -360) yAngle = -360;
	if (yAngle > 1200) yAngle = 1200;
    SetMousePosition(centerX, centerY);
}