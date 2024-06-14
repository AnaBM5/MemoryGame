#include "Button.h"
#include <string>

bool Button::Click(glm::vec2 coords)
{
	if (coords.x >= position.x - width2 && coords.x <= position.x + width2
		&& coords.y >= position.y - height2 && coords.y <= position.y + height2)
	{
		//collison!
		Activation();
		return true;
	}

	return false;
}

//Update returns a bool telling if the flashing animation is still playing or not
bool Button::Update(float seconds)
{
	if (lightUp)
	{		//transparency goes from 0 to 1 back to 0, giving "flashing" effect
			transparency += dir * static_cast<float>(seconds)*speed;
			if (transparency > 1)
			{
				dir *= -1;
				transparency = 1.f;
			}
			else if (transparency < 0)
			{
				transparency = 1.f;
				dir = 1;
				lightUp = false;
				return false;
			}
			return true;
			

	}
}

void Button::Draw()
{
	//if its animating, the transparency changes
	if (lightUp) 
	{
		sprites[1]->Blit(position.x, position.y,1.f ,1.f,transparency);
	}
	// if the player selected it, it inmediatly lights up
	else 
	{
		if (selected) sprites[1]->Blit(position.x, position.y);
		else sprites[0]->Blit(position.x, position.y);
	}
	
}

void Button::Activation()
{
	selected = true;
}

std::string  Button::Deactivate() //returns which button was just deactivated
{
	selected = false;
	return direction;
}

Button::~Button()
{

}

void Button::LightUp() 
{
	//Setups button for animation
	transparency = 0.f;
	lightUp = true;
}

bool Button::ChangeSpeed() 
{
	//ups the speed of the animation only up to a certain point so it doesnt become unfair
	//and returns if the speed was increased or not to speed other things in main
	if (speed < 5)
	{
		speed *= 1.25f;
		return true;
	}
	return false;
}