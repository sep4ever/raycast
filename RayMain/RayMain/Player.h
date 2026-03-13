#ifndef PLAYER_H
#define PLAYER_H

//#include "World.cpp"
#include "raylib.h"
#include "Settings.h"

class Player{
public:
    Player(Settings* settings);

    Settings* sets;
    
    float size = 40.0f;
    float radAngle = 0.0f;
    float yAngle = 0.0f;

    float speed = 30.0f;

    Vector2 position = { 0, 0 };
    Vector2 displacement = { 0, 0 };

    int rayCount = 120;
    float fov = 90 * (3.14159265f / 180.0f); // 90 градусов в радианах
    float rayLength = 300.0f;

    bool hitWall = false;
    float distance = 0.0f;

    float wallScale = 0.5f;

    void Move();
	void Rotate();
	float UpdateRays();
    
};


#endif