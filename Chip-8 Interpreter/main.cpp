#include <SDL.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <random>
#include <windows.h>

using ms = std::chrono::duration<double, std::milli>;

const int WINDOW_WIDTH = 64 * 10;
const int WINDOW_HEIGHT = 32 * 10;

const char* ROM_FILE_PATH = "C:\\rom.ch8";


SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Texture* texture;
uint32_t* frameBuffer;


/*********************CHIP-8*********************/

uint8_t RAM[4096 * 2] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F*/
};
uint16_t PC = 0x200;
uint16_t I;
uint8_t SP = 0;
uint16_t STACK[16] = {};
uint8_t DELAY_TIMER = 0;
uint8_t SOUND_TIMER = 0;
uint8_t V[16];

/***********************************************/

uint32_t bg = 0x0;
uint32_t fg = 0xFFFFFFFF;

uint16_t op_code, x, y, n, nnn, pos;
uint8_t bit, kk, firstOne, lastTwo;
int tmp;

uint8_t KEYBOARD[16] = {
	0x0, 0x0, 0x0, 0x0,	// 1-2-3-C
	0x0, 0x0, 0x0, 0x0, // 4-5-6-D
	0x0, 0x0, 0x0, 0x0, // 7-8-9-E
	0x0, 0x0, 0x0, 0x0  // A-0-B-F 
};

bool waitKey_Flag = false;
bool redraw_Flag = false;
bool collision = false;

void initRegs();
void printRegs();
int getPixel(int r, int c);
bool init();
void clearFrameBuffer();
void executeCycle();
void beep();


int getPixel(int row, int column) {
	return ( (row % 64) + (64 * (column % 32)) );
}

bool init(){
	/* TODO
		OPENFILENAME ofn;
		char file[255];

		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		// Initialize remaining fields of OPENFILENAME structure
		ofn.lStructSize = sizeof(ofn);
		//ofn.hwndOwner = hWnd;
		ofn.lpstrFile = file;
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = 255;

		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileName(&ofn))
		{
			std::cout << "Path: " << ofn.lpstrFile;

		}
	*/

	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("CHIP-8 Interpreter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				texture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888,
					SDL_TEXTUREACCESS_STREAMING, 64, 32);

				frameBuffer = new uint32_t[64 * 32]();
				initRegs();

				for (int i = 0; i < 64 * 32; i ++)
					frameBuffer[i] = bg ;		
  
				srand(time(NULL));
				

			}
		}
	}

	return success;
}

void clearFrameBuffer() {
	for (int i = 0; i < 64 * 32; i++)
		frameBuffer[i] = bg;

	SDL_UpdateTexture(texture, NULL, frameBuffer, 64 * sizeof(uint32_t));
}

void initRegs() {
	for (int i = 0; i < 16; i++)
		V[i] = 0x0;
}

void printRegs() {
	for (int i = 0; i < 16; i++)
		std::cout << "V[" << i << "]: " << std::hex << +V[i] << std::endl;

	std::cout << "PC: " << PC << std::endl;
	std::cout << "I: " << I << std::endl;
}

void printRam(int N) {
	for (uint16_t i = 0x200; i < 0x200 + N * 2; i+=2) 
		std::cout << "0x" << std::hex << i << "	"  << ((RAM[i] << 8) | RAM[i + 1]) << std::endl;
}



void executeCycle() {
	//Merge two bytes in a WORD
	op_code = (RAM[PC] << 8) | RAM[PC+1];
	PC += 2;

	//std::cout << "op_code: " << std::hex << op_code << std::endl;

	//Decode
	switch (op_code) {
		//CLS => Clear the display
		case 0x00E0:
			clearFrameBuffer();
			break;
		//RET => Return from a subroutine.
		case 0x00EE:
			PC = STACK[--SP];
			break;
		default:
			firstOne = (op_code & 0xF000) >> 12;
			switch (firstOne) {
			//JP addr => Jump to location nnn.
			case 0x1:
				nnn = op_code & 0x0FFF;
				PC = nnn;
				break;
			//CALL addr => Call subroutine at nnn.
			case 0x2:
				nnn = op_code & 0x0FFF;
				STACK[SP++] = PC;
				PC = nnn;
				break;
			//SE Vx, byte => Skip next instruction if Vx = kk.
			case 0x3:
				x = (op_code & 0x0F00) >> 8;
				kk = (op_code & 0x00FF);
				if (V[x] == kk) 
					PC += 2;
				break;
			//SNE Vx, byte => Skip next instruction if Vx != kk.
			case 0x4:
				x = (op_code & 0x0F00) >> 8;
				kk = (op_code & 0x00FF);
				if (V[x] != kk) 
					PC += 2;
				break;
			//SE Vx, Vy => Skip next instruction if Vx = Vy.
			case 0x5:
				x = (op_code & 0x0F00) >> 8;
				y = (op_code & 0x00F0) >> 4;
				if (V[x] == V[y])
					PC += 2;
				break;
			//LD Vx, bytekk => Set Vx = kk. The interpreter puts the value kk into register Vx.
			case 0x6:
				x = (op_code & 0x0F00) >> 8;
				kk = (op_code & 0x00FF);
				V[x] = kk;
				break;
			case 0x7:
				x = (op_code & 0x0F00) >> 8;
				kk = (op_code & 0x00FF);
				V[x] += kk;
				break;
			case 0x8:
				x = (op_code & 0x0F00) >> 8;
				y = (op_code & 0x00F0) >> 4;
				n = (op_code & 0x000F);
				switch (n) {
					case 0x0:
						V[x] = V[y];
						break;
					case 0x1:
						V[x] = V[x] | V[y];
						break;
					case 0x2:
						V[x] = V[x] & V[y];
						break;
					case 0x3:
						V[x] = V[x] ^ V[y];
						break;
					case 0x4:
						tmp = V[x] + V[y];
						V[0xF] = (tmp > 255) ? 1 : 0;
						V[x] = tmp & 0xFF;
						break;
					case 0x5:
						tmp = V[x] - V[y];
						V[x] = tmp;
						V[0xF] = (tmp > 0) ? 1 : 0;
						break;
					case 0x6:
						V[x] = V[x] >> 1;
						V[0xF] = ((V[x] & 0x01) != 0);
						break;
					case 0x7:
						tmp = V[y] - V[x];
						V[x] = tmp;
						V[0xF] = (tmp > 0) ? 1 : 0;
						break;
					case 0xE:
						V[0xF] = ((V[x] & 0x80) != 0);
						V[x] = V[x] << 1;
						break;
				}
				break;
			//SNE Vx, Vy => Skip next instruction if Vx != Vy.
			case 0x9:
				x = (op_code & 0x0F00) >> 8;
				y = (op_code & 0x00F0) >> 4;
				if (V[x] != V[y]) PC += 2;
				break;
			//LD I, addr => Set I = nnn.
			case 0xA:
				nnn = (op_code & 0x0FFF);
				I = nnn;
				break;
			//JP V0, addr => Jump to location nnn + V0.
			case 0xB:
				nnn = (op_code & 0x0FFF);
				PC = nnn + V[0];
				break;
			//RND Vx, byte => Set Vx = random byte AND kk.
			case 0xC:
				x = (op_code & 0x0F00) >> 8;
				kk = (op_code & 0x00FF);
				V[x] = (rand() % (0xFF + 1)) & kk;
				break;
			case 0xD:
				x = (op_code & 0x0F00) >> 8;
				y = (op_code & 0x00F0) >> 4;
				n = op_code & 0x000F;

				for (int j = 0; j < n; j++){
					for (int k = 0; k < 8; k++) {
						collision = (RAM[I+j] & (1 << (7-k))) != 0;
						if (collision) {
							pos = getPixel( (V[x] + k), (V[y] + j) );
							V[0xF] = (frameBuffer[pos] != 0);
							frameBuffer[pos] ^= fg;
						}
					}
				}

				SDL_UpdateTexture(texture, NULL, frameBuffer, 64 * sizeof(uint32_t));
				SDL_RenderClear(gRenderer);
				SDL_RenderCopy(gRenderer, texture, NULL, NULL);
				SDL_RenderPresent(gRenderer);
				break;
			
			case 0xE:
				x = (op_code & 0x0F00) >> 8;
				lastTwo = (op_code & 0x00FF);
				switch (lastTwo) {
					//SKP Vx => Skip next instruction if key with the value of Vx is pressed.
					case 0x9E:
						if (KEYBOARD[V[x]])
							PC += 2;
						break;
					//SKNP Vx => Skip next instruction if key with the value of Vx is not pressed.
					case 0xA1:
						if (!KEYBOARD[V[x]])
							PC += 2;
						break;
				}
				break;

			case 0xF:
				x = (op_code & 0x0F00) >> 8;
				lastTwo = (op_code & 0x00FF);
				switch (lastTwo) {
					//LD Vx, DT => Set Vx = delay timer value.
					case 0x07:
						V[x] = DELAY_TIMER;
						break;
					//LD Vx, K => Wait for a key press, store the value of the key in Vx.
					case 0x0A:
						for (int i = 0; i < 0xF; i++)
							if (KEYBOARD[i]) {
								V[x] = i;
								waitKey_Flag = true;
								break;
							}

						if (!waitKey_Flag)
							PC -= 2;
						break;
					//LD DT, Vx => Set delay timer = Vx.
					case 0x15:
						DELAY_TIMER = V[x];
						break;
					//LD ST, Vx => Set sound timer = Vx.
					case 0x18:
						SOUND_TIMER = V[x];
						break;
					//ADD I, Vx => Set I = I + Vx.
					case 0x1E:
						I = I + V[x];
						break;
					//LD F, Vx => Set I = location of sprite for digit Vx.
					case 0x29:
						I = V[x] * 5;
						break;
					//LD B, Vx => Store BCD representation of Vx in memory locations I, I + 1, and I + 2.
					case 0x33:
						//std::cout << "______________ " << std::hex << +V[x] << std::endl;
						RAM[I] = V[x] / 100;
						RAM[I + 1] = (V[x] / 10) % 10;
						RAM[I + 2] = V[x] % 10;
						break;
					//LD [I], Vx => Store registers V0 through Vx in memory starting at location I.
					case 0x55:
						for (int i = 0; i <= x; i++)
							RAM[I + i] = V[i];
						break;
					//LD Vx, [I] => Read registers V0 through Vx from memory starting at location I.
					case 0x65:
						for (int i = 0; i <= x; i++)
							V[i] = RAM[I + i];
						break;
				}
				break;


			}

	}

	if (DELAY_TIMER > 0)
		DELAY_TIMER--;

	if (SOUND_TIMER > 0) {
		if(SOUND_TIMER == 1)beep();
		SOUND_TIMER--;
	}

}

void beep() {
	//TODO
		/*std::cout << '\a' << std::flush;
		std::cout << '\a' << std::flush;*/
		//Beep(900, 50);
		//std::cout << '\a' << std::flush;
}

int main(int argc, char* args[])
{
	std::ifstream fin(ROM_FILE_PATH, std::ifstream::binary);

	for (int i = 0x200; !fin.eof() && i < 4096 * 2; i++) {
		char byte[1];
		fin.read(byte, 1);
		RAM[i] = (uint8_t) byte[0];
	}

	fin.close();



	if (!init())
		printf("Failed to initialize!\n");
	else {

		//Main loop flag
		bool quit = false;

		//Event handler
		SDL_Event e;

		//While application is running
		while (!quit)
		{

			do {
				//Handle events on queue
				while (SDL_PollEvent(&e) != 0) {
					//User requests quit
					if (e.type == SDL_QUIT)
						quit = true;
					else if (e.type == SDL_KEYDOWN) {
						switch (e.key.keysym.sym) {
							case SDLK_1: KEYBOARD[0x1] = 1; break;
							case SDLK_2: KEYBOARD[0x2] = 1; break;
							case SDLK_3: KEYBOARD[0x3] = 1; break;
							case SDLK_4: KEYBOARD[0xC] = 1; break;
							case SDLK_q: KEYBOARD[0x4] = 1; break;
							case SDLK_w: KEYBOARD[0x5] = 1; break;
							case SDLK_e: KEYBOARD[0x6] = 1; break;
							case SDLK_r: KEYBOARD[0xD] = 1; break;
							case SDLK_a: KEYBOARD[0x7] = 1; break;
							case SDLK_s: KEYBOARD[0x8] = 1; break;
							case SDLK_d: KEYBOARD[0x9] = 1; break;
							case SDLK_f: KEYBOARD[0xE] = 1; break;
							case SDLK_z: KEYBOARD[0xA] = 1; break;
							case SDLK_x: KEYBOARD[0x0] = 1; break;
							case SDLK_c: KEYBOARD[0xB] = 1; break;
							case SDLK_v: KEYBOARD[0xF] = 1; break;
						}
					}
					else if (e.type == SDL_KEYUP) {
						switch (e.key.keysym.sym) {
						case SDLK_1: KEYBOARD[0x1] = 0; break;
						case SDLK_2: KEYBOARD[0x2] = 0; break;
						case SDLK_3: KEYBOARD[0x3] = 0; break;
						case SDLK_4: KEYBOARD[0xC] = 0; break;
						case SDLK_q: KEYBOARD[0x4] = 0; break;
						case SDLK_w: KEYBOARD[0x5] = 0; break;
						case SDLK_e: KEYBOARD[0x6] = 0; break;
						case SDLK_r: KEYBOARD[0xD] = 0; break;
						case SDLK_a: KEYBOARD[0x7] = 0; break;
						case SDLK_s: KEYBOARD[0x8] = 0; break;
						case SDLK_d: KEYBOARD[0x9] = 0; break;
						case SDLK_f: KEYBOARD[0xE] = 0; break;
						case SDLK_z: KEYBOARD[0xA] = 0; break;
						case SDLK_x: KEYBOARD[0x0] = 0; break;
						case SDLK_c: KEYBOARD[0xB] = 0; break;
						case SDLK_v: KEYBOARD[0xF] = 0; break;
						}
					}
				}

			} while (waitKey_Flag && !quit);


			executeCycle();
	
			SDL_Delay(1);
			

		}
	}

	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;
	delete frameBuffer;

	SDL_Quit();

	return 0;
}