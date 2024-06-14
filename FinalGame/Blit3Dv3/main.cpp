/*
	Template example for Wwise audio event setup/loading/playback.
*/
#include "Blit3D.h"

Blit3D *blit3D = NULL;

//memory leak detection
#define CRTDBG_MAP_ALLOC
//Can't do the following if using Wwise in debug mode
/*
#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif  // _DEBUG
*/
#include <stdlib.h>
#include <crtdbg.h>

#include <random>
#include <fstream>
#include <iostream>

#include "Button.h"

#include "AudioEngine.h"

//GLOBAL DATA
std::mt19937 rng;

AngelcodeFont* mono72 = NULL;
AngelcodeFont* mono40 = NULL;
AngelcodeFont* mono40Yellow = NULL;

Sprite* blackOverlay = NULL;
Sprite* rightAnswer = NULL;
Sprite* wrongAnswer = NULL;

Sprite* panelSprite = NULL; // a pointer to the sprite of the base panel
Button* upButton = new Button;
Button* downButton = new Button;
Button* leftButton = new Button;
Button* rightButton = new Button;

std::vector<Button*> buttonList;		//list of the buttons to make updates and mouse easier
glm::vec2 mousePosition;
float waitingTime = 0.f;				//counter until the next button lights up
float roundWaitingTime = 0.3f;			//time to wait between buttons lighting up each round
float resultWaitingTime = 1.f;			//time the result is shown on screen
int showButton = 0;						//pointer to which button is the next to be lighted up
bool showPattern = false;				//to manage the update and draw when a sequence is showing so one element at a time is shown
Button* currentButton = NULL;			//references the button that needs to update when showing the sequence
int checkButton = 0;					//which button should the player be pressing

int lives = 3;
int level = 1;

enum class GameState { PLAYING, GAMEOVER, SHOWING, PAUSE, START, RIGHT, WRONG };
GameState gameState = GameState::START;

enum class Direction { UP, DOWN, LEFT, RIGHT };

std::uniform_int_distribution<int> buttonOptions(1, 4);
std::vector<Direction> patternList;						//list with the sequence the player must follow


AudioEngine * audioE = NULL;
AkGameObjectID mainGameID = 1;
AkPlayingID correctID, incorrectID, gameOverID,
			upButtonID, downButtonID, leftButtonID, rightButtonID;

bool soundPlayed = false;				//wheater the sound of the button has already played so it doesn't play more than once when showing pattern

void CreatePattern(bool firstIteration)
{
	//number of inputs added to the pattern
	int newButtonsAdded = 1;

	//if its happening for the first time, starts with adding three directions
	if (firstIteration)
		newButtonsAdded = 3;

	bool done = false;

	for (int i = 0; i < newButtonsAdded; i++)
	{
		//Selects random button to add to the sequence
		int newButton = buttonOptions(rng);
		switch (newButton)
		{
		case 1:
			patternList.push_back(Direction::UP);
			//std::cout << "Up";
			break;
		case 2:
			patternList.push_back(Direction::DOWN);
			//std::cout << "Down";
			break;
		case 3:
			patternList.push_back(Direction::LEFT);
			//std::cout << "Left";
			break;
		case 4:
			patternList.push_back(Direction::RIGHT);
			//std::cout << "Right";
			break;
		}

	}

}

void ShowPattern()
{
	//Lights up the button that needs to be shown next

	switch (patternList[showButton])
	{
	case Direction::UP:
		currentButton = upButton;
		break;
	case Direction::DOWN:
		currentButton = downButton;
		break;
	case Direction::LEFT:
		currentButton = leftButton;
		break;
	case Direction::RIGHT:
		currentButton = rightButton;
		break;
	}

	currentButton->LightUp();
	showPattern = true;
}

void NextButton()
{
	//if we haven't shown all the buttons yet, show the next one in the list, else start the player's turn
	if (showButton < patternList.size() - 1)
	{
		waitingTime = roundWaitingTime;
		showButton++;
		ShowPattern();
	}
	else {
		showButton = 0;
		gameState = GameState::PLAYING;
	}

}

bool CompareInput(std::string direction) {

	//Checks if the user input follows the sequence

	if (direction == "UP" && patternList[checkButton] == Direction::UP
		|| direction == "DOWN" && patternList[checkButton] == Direction::DOWN
		|| direction == "LEFT" && patternList[checkButton] == Direction::LEFT
		|| direction == "RIGHT" && patternList[checkButton] == Direction::RIGHT)
	{
		checkButton++;
		return true;
	}
	//if the player misses, they have to repeat the pattern from the beginning 
	checkButton = 0;
	return false;
}

void ClearButtons()
{
	for (auto& b : buttonList)
		b->selected = false;
}

void NewGame() {
	//Resets all the necessary variables to their original state
	lives = 3;
	level = 1;
	roundWaitingTime = 0.3f;
	showButton = 0;

	//clears the previous pattern and creates a new one
	patternList.clear();
	CreatePattern(true);

	//the speed of the buttons lighting up goes back to the original value
	for (auto& b : buttonList)
	{
		
		b->speed = 1.7f;
		b->selected = false;

	}
}


void Init()
{
	//seed rng
	std::random_device rd;
	rng.seed(rd());

	mono72 = blit3D->MakeAngelcodeFontFromBinary32("Media\\mono72.bin");
	mono40 = blit3D->MakeAngelcodeFontFromBinary32("Media\\mono40.bin");
	mono40Yellow = blit3D->MakeAngelcodeFontFromBinary32("Media\\mono40Yellow.bin");
	
	blackOverlay = blit3D->MakeSprite(0, 0, 960, 540, "Media\\blackOverlay.png");
	rightAnswer = blit3D->MakeSprite(0, 0, 300, 300, "Media\\rightAnswer.png");
	wrongAnswer = blit3D->MakeSprite(0, 0, 300, 300, "Media\\wrongAnswer.png");


	panelSprite = blit3D->MakeSprite(0, 0, 660, 440, "Media\\panelOff.png");
	//add individual sprite for buttons lightin up
	upButton->sprites[1] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\upOn.png");
	downButton->sprites[1] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\downOn.png");
	leftButton->sprites[1] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\leftOn.png");
	rightButton->sprites[1] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\rightOn.png");

	//add empty sprites for when buttons are not selected
	upButton->sprites[0] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\empty.png");
	downButton->sprites[0] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\empty.png");
	leftButton->sprites[0] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\empty.png");
	rightButton->sprites[0] = blit3D->MakeSprite(0, 0, 200, 200, "Media\\empty.png");

	//add position of the buttons
	upButton->position = glm::vec2(480, 300 + 100);
	downButton->position = glm::vec2(480, 300 - 110);
	leftButton->position = glm::vec2(480 - 216, 300 - 110);
	rightButton->position = glm::vec2(480 + 213, 300 - 110);

	//add directions to know which button the user is selecting
	upButton->direction = "UP";
	downButton->direction = "DOWN";
	leftButton->direction = "LEFT";
	rightButton->direction = "RIGHT";

	//setup buttons audioIds and audio Events for calling when they are clicked on
	upButton->eventName = "UpButton";
	downButton->eventName = "DownButton";
	leftButton->eventName = "LeftButton";
	rightButton->eventName = "RightButton";

	upButton->soundID = upButtonID;
	downButton->soundID = downButtonID;
	leftButton->soundID = leftButtonID;
	rightButton->soundID = rightButtonID;

	// adds all buttons to a list
	buttonList.push_back(upButton);
	buttonList.push_back(downButton);
	buttonList.push_back(leftButton);
	buttonList.push_back(rightButton);


	//create audio engine
	audioE = new AudioEngine;
	audioE->Init();
	audioE->SetBasePath("Media\\");

	//load banks
	audioE->LoadBank("Init.bnk");
	audioE->LoadBank("MainBank.bnk");

	//register our game objects
	audioE->RegisterGameObject(mainGameID);

	//Creates the first pattern
	CreatePattern(true);
	
}

void DeInit(void)
{
	if (audioE != NULL) delete audioE;
	for (auto* b : buttonList) 
	{
		if (b != NULL) delete b;
			
	}


}

void Update(double seconds)
{
	//must always update audio in our game loop
	audioE->ProcessAudio();


	if (gameState == GameState::SHOWING)
	{
		//Shows an element of the sequence and then pauses so they don't all appear at the same time
		ShowPattern();
		gameState = GameState::PAUSE;	
	}
	else if (gameState == GameState::RIGHT)
	{
		//Shows the user feedback for a small window of time before going to the next round
		if (waitingTime > 0) {
			waitingTime -= seconds;
		}
		else {
			level++;
			audioE->StopEvent("Correct", mainGameID, correctID);
			gameState = GameState::SHOWING;
		}
	}
	else if (gameState == GameState::WRONG)
	{
		//Shows the user feedback for a small window of time before showing the sequence again
		if (waitingTime > 0) {
			waitingTime -= seconds;
		}
		else {
			audioE->StopEvent("Incorrect", mainGameID, incorrectID);
			if (lives == 0)
			{
				//If no lives are left its Game Over
				gameOverID = audioE->PlayEvent("GameOver", mainGameID);
				gameState = GameState::GAMEOVER;
			}

			else
				gameState = GameState::SHOWING;
		}
	}
	if (showPattern) //Plays the flashing animation of a given button
	{
		//plays the sound around the middle of the animation where the button is the brigthest
		if (waitingTime <= roundWaitingTime / 2 && !soundPlayed)				
		{
			currentButton->soundID = audioE->PlayEvent(currentButton->eventName, mainGameID);
			soundPlayed = true;
		}
		if (waitingTime > 0) {
			waitingTime -= seconds;
		}
		else {
			//if animation is over, go to the next button
			if (!currentButton->Update(seconds))
			{
				showPattern = false;
				soundPlayed = false;
				NextButton();
			}

		}
	}
}

void Draw(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	//clear colour: r,g,b,a 	
	// wipe the drawing surface clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	if (gameState == GameState::START)
	{
		panelSprite->Blit(480, 300.f);
		blackOverlay->Blit(480.f, 300.f, 1.f, 1.f, 0.7f);


		std::string titleString = "FOLLOW THE PATTERN!";
		float widthTitle = mono72->WidthText(titleString);
		mono72->BlitText(480 - (widthTitle / 2), 450, titleString);

		std::string messageString = "Remember the pattern that shows up and";
		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(480 - (widthTitle / 2), 300, messageString);

		messageString = "copy it by using the arrow keys";
		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(480 - (widthTitle / 2), 270, messageString);

		messageString = "or pressing the buttons on the screen.";
		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(480 - (widthTitle / 2), 240, messageString);

		messageString = "Press any key to start the game";
		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(480 - (widthTitle / 2), 110, messageString);
	}

	if (gameState == GameState::GAMEOVER)
	{

		panelSprite->Blit(480, 300.f);
		blackOverlay->Blit(480.f, 300.f, 1.f, 1.f, 0.7f);


		std::string gameOverString = "GAME OVER";
		std::string numberRound = "Round " + std::to_string(level) + "!";
		std::string messageString = "You got up to ";



		float widthTitle = mono72->WidthText(gameOverString);
		mono72->BlitText(480 - (widthTitle / 2), 450, gameOverString);

		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(390 - (widthTitle / 2), 300, messageString);
		widthTitle = mono40Yellow->WidthText(numberRound);
		mono40Yellow->BlitText(650 - (widthTitle / 2), 300, numberRound);

		messageString = "Press any key to play again";
		widthTitle = mono40->WidthText(messageString);
		mono40->BlitText(480 - (widthTitle / 2), 150, messageString);


	}


	if (gameState == GameState::PLAYING || gameState == GameState::SHOWING || gameState == GameState::PAUSE
		|| gameState == GameState::RIGHT || gameState == GameState::WRONG)
	{
		panelSprite->Blit(480, 300.f);
		for (auto& b : buttonList)
		{
			b->Draw();

		}

		std::string roundString = "Round";
		std::string numberRound = std::to_string(level);

		float widthTitle = mono72->WidthText(roundString);
		mono72->BlitText(110 - (widthTitle / 2), 530, roundString);
		widthTitle = mono72->WidthText(numberRound);
		mono72->BlitText(110 - (widthTitle / 2), 480, numberRound);

		std::string livesString = "Lives";
		std::string numberLives = std::to_string(lives);

		widthTitle = mono72->WidthText(livesString);
		mono72->BlitText(850 - (widthTitle / 2), 530, livesString);
		widthTitle = mono72->WidthText(numberLives);
		mono72->BlitText(850 - (widthTitle / 2), 480, numberLives);

	}
	if (gameState == GameState::RIGHT)
	{
		blackOverlay->Blit(480.f, 300.f, 1.f, 1.f, 0.7f);
		rightAnswer->Blit(480.f, 280.f, 1.8f, 1.8f);
	}
	else if (gameState == GameState::WRONG)
	{
		blackOverlay->Blit(480.f, 300.f, 1.f, 1.f, 0.7f);
		wrongAnswer->Blit(480.f, 280.f, 1.8f, 1.8f);
	}


}

//the key codes/actions/mods for DoInput are from GLFW: check its documentation for their values
void DoInput(int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		blit3D->Quit(); //start the shutdown sequence
	if (gameState == GameState::PLAYING) {
		if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		{
			upButtonID = audioE->PlayEvent(upButton->eventName, mainGameID);
			upButton->Activation();
		}
		else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		{
			downButtonID = audioE->PlayEvent(downButton->eventName, mainGameID);
			downButton->Activation();
		}
		else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		{
			leftButtonID = audioE->PlayEvent(leftButton->eventName, mainGameID);
			leftButton->Activation();
		}
		else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		{
			rightButtonID = audioE->PlayEvent(rightButton->eventName, mainGameID);
			rightButton->Activation();
		}
			
		//Check if the input was right when the button is released
		if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
		{
			string direction = upButton->Deactivate();
			if (CompareInput(direction))
			{
				//If the whole sequence was done correctly, increases the speed of the animations
				// show the player they were right, and goes to the next round
				if (checkButton == patternList.size())
				{
					CreatePattern(false);
					checkButton = 0;

					for (auto& b : buttonList)
					{
						b->selected = false;
						if (b->ChangeSpeed()) roundWaitingTime /= 1.6f;
					}
					waitingTime = resultWaitingTime;

					correctID = audioE->PlayEvent("Correct", mainGameID);
					gameState = GameState::RIGHT;
				}

			}
			else
			{
				lives--;
				waitingTime = resultWaitingTime;
				incorrectID = audioE->PlayEvent("Incorrect", mainGameID);
				ClearButtons();
				gameState = GameState::WRONG;
			}
		}

		else if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
		{
			string direction = downButton->Deactivate();
			if (CompareInput(direction))
			{
				if (checkButton == patternList.size())
				{
					CreatePattern(false);
					checkButton = 0;

					for (auto& b : buttonList)
					{
						b->selected = false;
						if (b->ChangeSpeed()) roundWaitingTime /= 1.6f;
					}
					waitingTime = resultWaitingTime;

					correctID = audioE->PlayEvent("Correct", mainGameID);
					gameState = GameState::RIGHT;
				}

			}
			else
			{
				lives--;
				waitingTime = resultWaitingTime;
				ClearButtons();
				incorrectID = audioE->PlayEvent("Incorrect", mainGameID);
				gameState = GameState::WRONG;
			}
		}
		else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
		{
			string direction = leftButton->Deactivate();
			if (CompareInput(direction))
			{
				if (checkButton == patternList.size())
				{
					CreatePattern(false);
					checkButton = 0;

					for (auto& b : buttonList)
					{
						b->selected = false;
						if (b->ChangeSpeed()) roundWaitingTime /= 1.6f;
					}
					waitingTime = resultWaitingTime;

					correctID = audioE->PlayEvent("Correct", mainGameID);
					gameState = GameState::RIGHT;
				}

			}
			else
			{
				ClearButtons();
				lives--;
				waitingTime = resultWaitingTime;
				incorrectID = audioE->PlayEvent("Incorrect", mainGameID);
				gameState = GameState::WRONG;
			}
		}
		else if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
		{
			string direction = rightButton->Deactivate();
			if (CompareInput(direction))
			{
				if (checkButton == patternList.size())
				{
					CreatePattern(false);
					checkButton = 0;

					for (auto& b : buttonList)
					{
						b->selected = false;
						if (b->ChangeSpeed()) roundWaitingTime /= 1.6f;
					}
					waitingTime = resultWaitingTime;
					correctID = audioE->PlayEvent("Correct", mainGameID);
					gameState = GameState::RIGHT;
				}

			}
			else
			{
				lives--;
				waitingTime = resultWaitingTime;
				ClearButtons();
				incorrectID = audioE->PlayEvent("Incorrect", mainGameID);
				gameState = GameState::WRONG;
			}
		}
	}
	if (gameState == GameState::START)
	{
		if (action == GLFW_RELEASE)
			gameState = GameState::SHOWING;


	}
	if (gameState == GameState::GAMEOVER)
	{
		if (action == GLFW_RELEASE)
		{
			NewGame();
			audioE->StopEvent("GameOver", mainGameID, gameOverID);
			gameState = GameState::START;
		}

	}


	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		//We can play events by name:
		correctID = audioE->PlayEvent("Correct", mainGameID);
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
	{
		//We can stop events by playing special "stop" events:
		audioE->StopEvent("Correct", mainGameID, correctID);
	}
	else if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		//We can pase/resume events:
		//if (!drumsPaused);
			//audioE->PauseEvent("Drums", mainGameID, drumsID);
		//else
			//audioE->ResumeEvent("Drums", mainGameID, drumsID);
	}
	else if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		gameOverID = audioE->PlayEvent("GameOver", mainGameID);
	}
	else if (key == GLFW_KEY_E && action == GLFW_RELEASE)
	{
		audioE->StopEvent("GameOver", mainGameID, gameOverID);
	}
}

//called whenever the user resizes the window
void DoResize(int width, int height)
{
	blit3D->trueScreenWidth = blit3D->screenWidth = static_cast<float>(width);
	blit3D->trueScreenHeight = blit3D->screenHeight = static_cast<float>(height);
	blit3D->Reshape(blit3D->shader2d);
}

void DoCursor(double x, double y)
{
	mousePosition.x = (float)x;
	mousePosition.y = blit3D->trueScreenHeight - (float)y; //invert y for Blit3D screen coords

	//scale, in case screen resolution does not match our mode
	mousePosition.x = mousePosition.x * (blit3D->screenWidth / blit3D->trueScreenWidth);
	mousePosition.y = mousePosition.y * (blit3D->screenHeight / blit3D->trueScreenHeight);
}

void DoMouseButton(int button, int action, int mods)
{
	if (gameState == GameState::START)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
			gameState = GameState::SHOWING;
	}
	else if (gameState == GameState::GAMEOVER)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			NewGame();
			audioE->StopEvent("GameOver", mainGameID, gameOverID);
			gameState = GameState::START;
		}

	}
	else if (gameState == GameState::PLAYING) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			for (auto& b : buttonList)
			{
				if (b->Click(mousePosition))
				{
					b->soundID = audioE->PlayEvent(b->eventName, mainGameID);
					break;
				}
			}
		}
		else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
			for (auto& b : buttonList)
			{
				if (b->selected)
				{
					string direction = b->Deactivate();
					if (CompareInput(direction))
					{
						if (checkButton == patternList.size())
						{
							CreatePattern(false);
							checkButton = 0;

							for (auto& b : buttonList)
							{
								b->selected = false;
								if (b->ChangeSpeed()) roundWaitingTime /= 1.6f;
							}
							waitingTime = resultWaitingTime;
							correctID = audioE->PlayEvent("Correct", mainGameID);
							gameState = GameState::RIGHT;
						}

					}
					else
					{
						lives--;
						waitingTime = resultWaitingTime;
						ClearButtons();
						incorrectID = audioE->PlayEvent("Incorrect", mainGameID);
						gameState = GameState::WRONG;
					}

				}
			}
		}
	}


}

int main(int argc, char *argv[])
{
	//memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	blit3D = new Blit3D(Blit3DWindowModel::DECORATEDWINDOW, 1920 / 2, 1080 / 2);

	//set our callback funcs
	blit3D->SetInit(Init);
	blit3D->SetDeInit(DeInit);
	blit3D->SetUpdate(Update);
	blit3D->SetDraw(Draw);
	blit3D->SetDoInput(DoInput);
	blit3D->SetDoResize(DoResize);

	//add the mouse callbacks
	blit3D->SetDoCursor(DoCursor);
	blit3D->SetDoMouseButton(DoMouseButton);

	//Run() blocks until the window is closed
	blit3D->Run(Blit3DThreadModel::SINGLETHREADED);
	if (blit3D) delete blit3D;
}