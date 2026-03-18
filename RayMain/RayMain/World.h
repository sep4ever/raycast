#pragma once
#include <raylib.h>
#include <vector>

class Player;

class World {
	public:
		World(Player* player);
		Player* p;
		void Render(int rayCount, bool hitWall, float distance, float projPlaneDist);
		void DrawMinimap();
		void DrawFloorCeiling(float projPlaneDist);
		void RayTraceScene(int width, int height);
		float SphereIntersect(Vector3 ro, Vector3 rd, Vector3 center, float radius);
		//float IntersectCircle2D(float ox, float oy, float dx, float dy, float cx, float cy, float r);
};