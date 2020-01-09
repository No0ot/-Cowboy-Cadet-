// Created by Chris Tulip, enemies spawn randomly between the 2 platforms and take 3 shots to kill
// Move all the way to the right to scroll
#include <iostream>
#include <vector>
#include <algorithm>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <cmath>
#include <ctime>
#include <stdlib.h>
#define FPS 60
#define WIDTH 1024
#define HEIGHT 768

// Global variables
bool g_bRunning = false;
Uint32 g_start, g_end, g_delta, g_fps;
int g_iSpeed = 8; // speed of actor
const Uint8* g_iKeystates; // state of keyboard keys
SDL_Window* g_pWindow; // the window
SDL_Renderer* g_pRenderer; // buffer to draw to
bool ifJumping = false;
int jumpvalue = 20;
int gravity = 1;
bool facingRight = true;
int enemyspawnMAX = 300
	, enemyspawn = 90;
int score = 0;
int plHealth = 3;
bool alive = true;
Mix_Music* g_pMusic;
int g_pchannel = 1;
Mix_Chunk* g_aSound;
Mix_Chunk* g_eaSound;
Mix_Chunk* g_dthSound;

//animation variables
int g_frame[10] = { 0, 0 , 0, 0,0};
int g_frameMAX[10] = { 6, 6, 6, 6,6 };
int g_sprite[10] = { 0, 0, 0, 0,0 };
int g_spriteMax[10] = {10, 10, 10, 10,10 };

//sprite variables
SDL_Rect g_PLsrc, g_PLdst
, g_BGsrc, g_BGdst[3]
, g_FLsrc[5], g_FLdst[5]
, g_ENsrc, g_BBdst[5]
, g_dthsrc, g_dthdst;



bool collisionCheck(SDL_Rect r1, SDL_Rect r2)
{
	return (!(r1.y >= (r2.y + r2.h) || (r1.y + r1.h) <= r2.y ||
		   	r1.x >= (r2.x + r2.w) || (r1.x + r1.w) <= r2.x));

}

bool headCheck(SDL_Rect r1, SDL_Rect r2)
{
	return (!(r1.y >= (r2.y + (r2.h / 10)) || (r1.y + r1.h) <= r2.y ||
			r1.x >= (r2.x + r2.w) || (r1.x + r1.w) <= r2.x));

}


void animate(SDL_Rect& sprite, int i)
{
	if (g_frame[i] == g_frameMAX[i])
	{
		g_frame[i] = 0;
		g_sprite[i]++;
		if (g_sprite[i] == g_spriteMax[i])
			g_sprite[i] = 0;
		sprite.x = sprite.w * g_sprite[i];

	}
	g_frame[i]++;
}


//Class for bullet, make a header for this later
class Arrow 
{
private:
	
public:      //properties
	bool m_active = true;
	bool f = true;
	SDL_Rect  m_src , m_dst;
	static const int speed = 5; // static const makes a shared property across all instances
	//METHODS
	Arrow(int x, int y)
	{
		m_dst = {x-2, y-2, 32*2 , 32*2 };
		m_src = { 0, 0, 32, 32 };

		if (facingRight)
			f = true;
		else
			f = false;
	}
	void update()
	{
		if (f == true) {
			m_dst.x += speed * 3;
			if (m_dst.x > WIDTH)  // if bullet goes off screen
				m_active = false;
		}
		else {
			m_dst.x -= speed * 3;
			if (m_dst.x < - m_dst.w)  // if bullet goes off screen
				m_active = false;
		}
	}
};
// Vector for arrows
std::vector<Arrow*> arrowVec;
// Class for enemeny bullets, not sure if this is entirely necessary but it certaintly makes it easier
class EnArrow {
public:
	bool m_active = true;
	SDL_Rect  m_src, m_dst;
	static const int speed = 5;
	
	int m_channel;

	EnArrow(int x, int y)
	{
		m_dst = { x - 2, y - 2, 32 * 2 , 32 * 2 };
		m_src = { 0, 0, 32, 32 };
	
	}
	void update()
	{
		m_dst.x -= speed * 3;
		if (m_dst.x < -10)  // if bullet goes off screen
			m_active = false;
	}
};

std::vector<EnArrow*> enarrowVec;

class Enemy {
public:
	bool m_active = true;
	SDL_Rect m_src, m_dst;
	static const int speed = 3;
	int health = 5,
		arrowspawn = 0,
		arrowspawnMAX = 100;
	

	// methods
	Enemy(int x, int y)
	{
		m_dst = { x , y , 32 * 3 , 50 * 3 };
		m_src = { 0, 0, 32, 50 };
	}

	void update()
	{
		if (!collisionCheck(m_dst, g_BBdst[0]) && !collisionCheck(m_dst, g_BBdst[2]))
		{
			m_dst.y += g_iSpeed;
		}
		if (arrowspawn == arrowspawnMAX) {
			arrowspawn = 0;
			enarrowVec.push_back(new EnArrow(m_dst.x, m_dst.y + m_dst.h / 3));
			Mix_PlayChannel(1, g_eaSound, 0);
			
		}
		arrowspawn++;
		
		for (int i = 0; i < (int)arrowVec.size(); i++) {
			
			if (headCheck(arrowVec[i]->m_dst, m_dst)) {
				health -= 5;
				arrowVec[i]->m_active = false;
				Mix_PlayChannel(2, g_dthSound, 0);
			}
			if (collisionCheck(arrowVec[i]->m_dst, m_dst)) {
				health--;
				arrowVec[i]->m_active = false;
				Mix_PlayChannel(2, g_dthSound, 0);
			}
		}
		m_dst.x -= speed;
		if (m_dst.x < -10)  
			m_active = false;
		if (health <= 0) {
			m_active = false;
			score++;
			
		}
	}
};
// Vector for enemys
std::vector<Enemy*> enemyVec;

SDL_Texture* g_pTexture[10];
void updatePanning()
{
	Mix_SetPanning(0, 0, 255); // Full right.
	Mix_SetPanning(1, 64, 128); // Partly right.
	Mix_SetPanning(2, 128, 64); // Partly left.
	Mix_SetPanning(3, 255, 0); // Full left.
}


bool init(const char* title, int xpos, int ypos, int width, int height, int flags)
{
	//initialize SDL
	std::cout << "Initializing game." << std::endl;
	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)   // everything is okay
	{
		//create the window
		g_pWindow = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
		if (g_pWindow != nullptr)
		{

			g_pRenderer = SDL_CreateRenderer(g_pWindow, -1, 0);
			if (g_pRenderer != nullptr)
			{
				//initialize subsystems
				// RNG for spawn
				srand((unsigned)time(NULL));
				

				if (IMG_Init(IMG_INIT_PNG))
				{
					g_pTexture[1] = IMG_LoadTexture(g_pRenderer, "Darkranger.png");
					g_pTexture[2] = IMG_LoadTexture(g_pRenderer, "Arrow.png");
					g_pTexture[3] = IMG_LoadTexture(g_pRenderer, "forest.png");
					g_pTexture[4] = IMG_LoadTexture(g_pRenderer, "EnemyRun.png");
					g_pTexture[5] = IMG_LoadTexture(g_pRenderer, "Ground.png");
					g_pTexture[6] = IMG_LoadTexture(g_pRenderer, "RangerDeath.png");
				}
				if (Mix_Init(MIX_INIT_MP3) != 0) // Mixer init success.
				{
					Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 8192);
					Mix_AllocateChannels(8); 
					g_pMusic = Mix_LoadMUS("Heroic Demise.mp3");
					g_aSound = Mix_LoadWAV("shoot.wav");
					g_eaSound = Mix_LoadWAV("Archers-shooting.wav");
					g_dthSound = Mix_LoadWAV("pain1.wav");
					Mix_Volume(-1, MIX_MAX_VOLUME); // Volume for all channels.
					Mix_VolumeMusic(MIX_MAX_VOLUME / 2); // Volume for channel 0 to 64.
					updatePanning();
				}

				else return false;


			}
			else return false;
		}
		else return false;
	}
	else return false;
	//everything is cool start engine
	g_fps = (Uint32) round((1.0 / (double)FPS) * 1000.0);
	g_iKeystates = SDL_GetKeyboardState(nullptr);
	
	//Creating sprite rectangles
	// Character
	g_PLsrc = { 0, 0, 32, 32 };
	g_PLdst = { 200, 500, g_PLsrc.w * 4, g_PLsrc.h * 4 };
	g_dthsrc = { 0, 0, 32, 32 };
	g_dthdst = { g_PLdst.x, g_PLdst.y, g_dthsrc.w * 4, g_dthsrc.h * 4 };

	 //background
	g_BGsrc = { 0, 0, 1184, 800 };
	g_BGdst[0] = { 0 , height- g_BGsrc.h  , g_BGsrc.w, g_BGsrc.h };
	g_BGdst[1] = {  1184, height - g_BGsrc.h  , g_BGsrc.w, g_BGsrc.h };
	g_BGdst[2] = { -1184, height - g_BGsrc.h  , g_BGsrc.w, g_BGsrc.h };


	//ground/ platform
	g_FLsrc[0] = { 0, 0, 1024, 32 };
	g_FLdst[0] = { 0, 680, g_FLsrc[0].w * 2 , g_FLsrc[0].h * 3};
	g_BBdst[0] = { 0, 730, g_FLsrc[0].w * 2 , g_FLsrc[0].h * 2 };
	g_FLdst[1] = { 2048, 680, g_FLsrc[0].w * 2 , g_FLsrc[0].h * 3 };
	g_FLdst[3] = { -2048, 680, g_FLsrc[0].w * 2 , g_FLsrc[0].h * 3 };
	g_FLdst[2] = { 600, 500, g_FLsrc[0].w * 2 , g_FLsrc[0].h * 2 }; // platform
	g_BBdst[2] = { 620, 525, g_FLsrc[0].w * 2 , g_FLsrc[0].h };

	//sound stuff
	Mix_PlayMusic(g_pMusic, -1);
	
	
	

	g_bRunning = true;
	std::cout << "Success!" << std::endl;
	return true;

}

void wake() {
	// Records milliseconds since start
	g_start = SDL_GetTicks();
}
void sleep() {
	// Gets ticks and milliseconds and makes engine sleep for difference
	// This creates FPS 
	g_end = SDL_GetTicks();
	g_delta = g_end - g_start;
	if (g_delta < g_fps) {
		SDL_Delay(g_fps - g_delta);
	}
}

void spawnBullet() {
	if (facingRight)
		arrowVec.push_back(new Arrow(g_PLdst.x + 100, g_PLdst.y + g_PLdst.h / 3));
	else
		arrowVec.push_back(new Arrow(g_PLdst.x, g_PLdst.y + g_PLdst.h / 3));

	
	Mix_PlayChannel(0, g_aSound, 0);
}

void handleEvents() {

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type) {
		case SDL_QUIT:
			g_bRunning = false;
		case SDL_KEYDOWN:  // try SDL_KEYUP as well
			if (event.key.keysym.sym == SDLK_ESCAPE)
				g_bRunning = false;
			
			else if (event.key.keysym.sym == SDLK_SPACE) {
				// create bullet
				
					spawnBullet();
				
			}
			break;
		}
	}
}

bool keyDown(SDL_Scancode c)
{
	// Checking key presses with bool value
	if (g_iKeystates != nullptr)
	{
		if (g_iKeystates[c] == 1)
			return true;
		else return false;
	}
	return false;

}


void EnemySpawn() {
	int randomNum = 1 + rand() % 2;
	if (randomNum == 1) {
		if (enemyspawn == enemyspawnMAX) {
			enemyspawn = 0;
			enemyVec.push_back(new Enemy(1180, g_FLdst[0].y - 100));
		}
		enemyspawn++;
	}

	else if (randomNum == 2) {
		if (enemyspawn == enemyspawnMAX) {
			enemyspawn = 0;
			enemyVec.push_back(new Enemy(1180, g_FLdst[2].y - 100));
		}
		enemyspawn++;
	}
}


void update() {

	updatePanning();
	// Player/ enemy arrow collision
	for (int i = 0; i < (int)enarrowVec.size(); i++) {

		if (collisionCheck(enarrowVec[i]->m_dst, g_PLdst)) {
			plHealth--;
			enarrowVec[i]->m_active = false;
			Mix_PlayChannel(2, g_dthSound, 0);
		}
		if (plHealth <= 0)
			alive = false;
	}

	// Player/ Enemy collision
	for (int i = 0; i < (int)enemyVec.size(); i++) {

		if (collisionCheck(enemyVec[i]->m_dst, g_PLdst)) {
			plHealth--;
			enemyVec[i]->m_active = false;
			Mix_PlayChannel(2, g_dthSound, 0);
		}
		if (plHealth <= 0)
			alive = false;
	}
	if (!alive) {
		animate(g_dthsrc, 4);
	}

	for (int i = 0; i < (int)enemyVec.size(); i++) {
		animate(enemyVec[i]->m_src, i);
	}

	// constant gravity
	if (!ifJumping && !collisionCheck(g_PLdst, g_BBdst[0]) && !collisionCheck(g_PLdst, g_BBdst[2]))
	{
		g_PLdst.y += g_iSpeed;
	}


	// Jumping!!
	if (keyDown(SDL_SCANCODE_W) && !ifJumping)
		ifJumping = true;

	if (ifJumping)
	{
		g_PLdst.y -= jumpvalue;
		jumpvalue -= gravity;

	}
	if (collisionCheck(g_PLdst, g_BBdst[0]) || collisionCheck(g_PLdst, g_BBdst[2]))
	{
		ifJumping = false;
		jumpvalue = 20;

	}
	// make a crouch
	if (g_PLdst.y < 600) {
		if (keyDown(SDL_SCANCODE_S)) {
			g_PLdst.y += g_iSpeed;

		}
	}

	if (g_PLdst.x > 10) {
		if (keyDown(SDL_SCANCODE_A)) {
			animate(g_PLsrc, 3);
			g_PLdst.x -= g_iSpeed;
			facingRight = false;
		}


	}
	if (g_PLdst.x <= 850) {
		if (keyDown(SDL_SCANCODE_D)) {
			animate(g_PLsrc, 3);
			g_PLdst.x += g_iSpeed;
			facingRight = true;

		}
	}
	// Background and Ground scroll
	if (keyDown(SDL_SCANCODE_D) && g_PLdst.x >= 850) {
		animate(g_PLsrc, 3);
		g_BGdst[0].x -= g_iSpeed;
		g_BGdst[1].x -= g_iSpeed;
		g_BGdst[2].x -= g_iSpeed;
		g_FLdst[0].x -= g_iSpeed;
		g_FLdst[1].x -= g_iSpeed;
		g_FLdst[3].x -= g_iSpeed;
	}

	if (keyDown(SDL_SCANCODE_A) && g_PLdst.x <= 50) {
		animate(g_PLsrc, 3);
		g_BGdst[0].x += g_iSpeed;
		g_BGdst[1].x += g_iSpeed;
		g_BGdst[2].x += g_iSpeed;
		g_FLdst[0].x += g_iSpeed;
		g_FLdst[1].x += g_iSpeed;
		g_FLdst[3].x += g_iSpeed;
	}

	if (g_BGdst[0].x == -1184 || g_BGdst[0].x == 1184) {
		g_BGdst[0].x = 0;
		g_BGdst[1].x = 1184;
		g_BGdst[2].x = -1184;
	}

	if (g_FLdst[0].x == -2048 || g_FLdst[0].x == 2048) {
		g_FLdst[0].x = 0;
		g_FLdst[1].x = 2048;
		g_FLdst[3].x = -2048;
	}

	// Projectile Handling
	for (int i = 0; i < (int)arrowVec.size(); i++)
	{
		arrowVec[i]->update();
		if (arrowVec[i]->m_active == false)
		{
			delete arrowVec[i];
			arrowVec[i] = nullptr;
		}
	}

	if (!arrowVec.empty())
	{
		arrowVec.erase(remove(arrowVec.begin(), arrowVec.end(), nullptr), arrowVec.end());
		arrowVec.shrink_to_fit();
	}

	// Enemy projectile handling
	for (int i = 0; i < (int)enarrowVec.size(); i++)
	{
		enarrowVec[i]->update();
		if (enarrowVec[i]->m_active == false)
		{
			delete enarrowVec[i];
			enarrowVec[i] = nullptr;
		}
	}

	if (!enarrowVec.empty())
	{
		enarrowVec.erase(remove(enarrowVec.begin(), enarrowVec.end(), nullptr), enarrowVec.end());
		enarrowVec.shrink_to_fit();
	}
	std::cout << score << std::endl;

	// Enemy spawning

	if (enemyVec.size() <= 4)
		EnemySpawn();


	// Enemy handling
	for (int i = 0; i < (int)enemyVec.size(); i++)
	{
		enemyVec[i]->update();
		if (enemyVec[i]->m_active == false)
		{
			delete enemyVec[i];
			enemyVec[i] = nullptr;
		}
	}
	if (!enemyVec.empty())
	{
		enemyVec.erase(remove(enemyVec.begin(), enemyVec.end(), nullptr), enemyVec.end());
		enemyVec.shrink_to_fit();
	}

}

void render()
{
	// Clears screen by painting over every frame
	SDL_SetRenderDrawColor(g_pRenderer, 0, 0, 0, 255);// R,G,B, Alpha
	SDL_RenderClear(g_pRenderer);

	// Background Render
	SDL_RenderCopy(g_pRenderer, g_pTexture[3], &g_BGsrc, &g_BGdst[0]);
	SDL_RenderCopy(g_pRenderer, g_pTexture[3], &g_BGsrc, &g_BGdst[1]);
	SDL_RenderCopy(g_pRenderer, g_pTexture[3], &g_BGsrc, &g_BGdst[2]);
	// Ground/ Platform Render
	SDL_RenderCopy(g_pRenderer, g_pTexture[5], &g_FLsrc[0], &g_FLdst[0]);
	SDL_RenderCopy(g_pRenderer, g_pTexture[5], &g_FLsrc[0], &g_FLdst[1]);
	SDL_RenderCopy(g_pRenderer, g_pTexture[5], &g_FLsrc[0], &g_FLdst[2]);
	SDL_RenderCopy(g_pRenderer, g_pTexture[5], &g_FLsrc[0], &g_FLdst[3]);

	// Player Character Render
	if (alive)
	{
		SDL_RenderCopyEx(g_pRenderer, g_pTexture[1], &g_PLsrc, &g_PLdst, 0.0, nullptr, (facingRight == false ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));
	}
	else if (!alive)
	{
		SDL_RenderCopyEx(g_pRenderer, g_pTexture[6], &g_PLsrc, &g_PLdst, 0.0, nullptr, (facingRight == false ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));
	}
	
	// Enemy Render
	for (int i = 0; i < (int)enemyVec.size(); i++)
	{
		SDL_RenderCopyEx(g_pRenderer, g_pTexture[4], &enemyVec[i]->m_src, &enemyVec[i]->m_dst,0.0,nullptr, SDL_FLIP_HORIZONTAL);
	}
	// Projectile Render
	for (int i = 0; i < (int)arrowVec.size(); i++)
	{
		SDL_RenderCopyEx(g_pRenderer, g_pTexture[2], &arrowVec[i]->m_src, &arrowVec[i]->m_dst, 0.0, nullptr, (arrowVec[i]-> f == false ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE));
	}
	for (int i = 0; i < (int)enarrowVec.size(); i++)
	{
		SDL_RenderCopyEx(g_pRenderer, g_pTexture[2], &enarrowVec[i]->m_src, &enarrowVec[i]->m_dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
	}
	// Draw anew
	SDL_RenderPresent(g_pRenderer); // Sends data from buffer to screen
}

void clean() {
	// Closes everything down
	std::cout << "Cleaning Game." << std::endl;
	for (int i = 0; i < 5; i++) {
		SDL_DestroyTexture(g_pTexture[i]);
	}
	Mix_CloseAudio();
	Mix_FreeMusic(g_pMusic);
	SDL_DestroyRenderer(g_pRenderer);
	SDL_DestroyWindow(g_pWindow);
	SDL_Quit();

}

int main(int argc, char* argv[])
{
	if (init("Game1007 SDL Setup", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0) == false) 
		return 1;
	while (g_bRunning) {

		wake();
		handleEvents();
		update();
		render();

		if (g_bRunning)
			sleep();
		
	}
	
	
	clean();
	return 0;
	system("PAUSE");
	
}