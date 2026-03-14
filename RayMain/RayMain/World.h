#pragma once

class Player;

class World {
	public:
		World(Player* player);
		Player* p;
		void Render(int rayCount, bool hitWall, float distance, float projPlaneDist);
		void DrawMinimap();
		void DrawFloorCeiling(float projPlaneDist);
		//float IntersectCircle2D(float ox, float oy, float dx, float dy, float cx, float cy, float r);
};