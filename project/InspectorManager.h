#pragma once

#include<list>

class Particle;

class InspectorManager
{
public:
	InspectorManager();
	~InspectorManager();

	void RegisterItem(Particle& particle);

	void Draw();

private:
};

