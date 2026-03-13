#pragma once

class Player;

class World {
	public:
		World(Player* player);
		Player* p;
		void Render(int rayCount, bool hitWall, float distance, float projPlaneDist);
};