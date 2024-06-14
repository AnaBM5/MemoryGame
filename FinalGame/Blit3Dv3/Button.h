#pragma once
#include "Blit3D.h"
#include <string>
#include "AudioEngine.h"

class Button
{
public:
	Sprite* sprites[2]; //one sprite for selected, one for unselected
	std::string direction;


	glm::vec2 position; //center locaton of the button
	float width2 = 100, height2 = 100;
	bool selected = false;		//is the player clicking it
	bool lightUp = false;		//is it currently being shown as part of a pattern

	float speed = 1.7f;			//how fast the button lights up and turns off again
	
	float transparency = 1.f;
	int dir = 1;

	AkPlayingID soundID;
	std::string eventName;

	bool Click(glm::vec2 coords); //return true of this button was clicked on
	bool Update(float seconds);
	void Draw();

	virtual void Activation();
	std::string Deactivate();

	virtual void LightUp();
	virtual bool ChangeSpeed();	//makes the animation faster with each new round up to a certain point

	virtual ~Button();
};
