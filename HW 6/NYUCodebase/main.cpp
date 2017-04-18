#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <stdlib.h>
#include <windows.h> 
#include "ShaderProgram.h"
#include "Matrix.h"
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure to path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return retTexture;
}

class Paddle {
public:
	Paddle(float x1, float y1, float x2, float y2) : left(x1), top(y1), right(x2), bottom(y2) {}
	float left;
	float top;
	float right;
	float bottom;
};

class Ball {
public:
	Ball(float x1, float y1, float x2, float y2, float dir1, float dir2, float spd) : left(x1), top(y1), right(x2), bottom(y2), dirX(dir1), dirY(dir2), speed(spd) {}
	float left;
	float top;
	float right;
	float bottom;
	float dirX;
	float dirY;
	float speed;
};

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Copyrighted Music Version of Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	// Settop
	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint bg = LoadTexture(RESOURCE_FOLDER"Textures/background.png");
	GLuint winner = LoadTexture(RESOURCE_FOLDER"Textures/winner.png");

	Matrix ball;
	Matrix lPaddle;
	Matrix rPaddle;
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	float angle = float((rand() % 4) * 90 + 45) * 3.1415f / 180.0f;
	Ball aBall(0.0f, 0.14f, 0.14f, 0.0f, cos(angle), sin(angle), 1.0f);
	Paddle leftPaddle(-3.55f, 0.56f, -3.41f, 0.0f);
	Paddle rightPaddle(3.41f, 0.56f, 3.55f, 0.0f);

	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0f;
	// End Setup

	float aVertices[] = { -3.55f, -2.0f, 3.55f, -2.0f, 3.55f, 2.0f, -3.55f, -2.0f, 3.55f, 2.0f, -3.55f, 2.0f };
	float bVertices[] = { -3.55f, 0.0f, -3.41f, 0.0f, -3.41f, 0.56f, -3.55f, 0.0f, -3.41f, 0.56f, -3.55f, 0.56f };
	float cVertices[] = { 3.41f, 0.0f, 3.55f, 0.0f, 3.55f, 0.56f, 3.41f, 0.0f, 3.55f, 0.56f, 3.41f, 0.56f };
	float dVertices[] = { 0.0f, 0.0f, 0.14f, 0.0f, 0.14f, 0.14f, 0.0f, 0.0f, 0.14f, 0.14f, 0.0f, 0.14f };
	float e1Vertices[] = { -3.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -3.0f, -1.0f, -1.0f, 1.0f, -3.0f, 1.0f };
	float e2Vertices[] = { 1.0f, -1.0f, 3.0f, -1.0f, 3.0f, 1.0f, 1.0f, -1.0f, 3.0f, 1.0f, 1.0f, 1.0f };

	float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Chunk *hit = Mix_LoadWAV("Sounds/hit.wav");
	Mix_Chunk *bounce = Mix_LoadWAV("Sounds/bounce.wav");
	Mix_Chunk *yay = Mix_LoadWAV("Sounds/yay.wav");
	Mix_Music *bgm = Mix_LoadMUS("Sounds/illegalcopyrightedsong.mp3");

	Mix_PlayMusic(bgm, -1);

	// Enable Keyboard Inputs
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		// Time Tracking
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		// Drawing
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		// Background
		glBindTexture(GL_TEXTURE_2D, bg);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Background

		// Left Paddle
		lPaddle.identity();
		if (keys[SDL_SCANCODE_W] && leftPaddle.top < 1.86f) {
			leftPaddle.top += 4.0f * elapsed;
			leftPaddle.bottom += 4.0f * elapsed;
		} else if (keys[SDL_SCANCODE_S] && leftPaddle.bottom > -2.0f) {
			leftPaddle.top -= 4.0f* elapsed;
			leftPaddle.bottom -= 4.0f * elapsed;
		}
		lPaddle.Translate(0.0, leftPaddle.bottom, 0.0);
		program.setModelMatrix(lPaddle);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		// End Left Paddle

		// Right Paddle
		rPaddle.identity();
		if (keys[SDL_SCANCODE_UP] && rightPaddle.top < 1.86f) {
			rightPaddle.top += 4.0f * elapsed;
			rightPaddle.bottom += 4.0f * elapsed;
		} else if (keys[SDL_SCANCODE_DOWN] && rightPaddle.bottom > -2.0f) {
			rightPaddle.top -= 4.0f * elapsed;
			rightPaddle.bottom -= 4.0f * elapsed;
		}
		rPaddle.Translate(0.0f, rightPaddle.bottom, 0.0f);
		program.setModelMatrix(rPaddle);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		// End Right Paddle

		// Ball
		ball.identity();
		if (aBall.top > 1.86f && aBall.dirY > 0.0f || aBall.bottom < -2.0f && aBall.dirY < 0.0f) { // Top and Bottom collision
			Mix_PlayChannel(-1, bounce, 0);
			aBall.dirY *= -1.0f;
		} else if (aBall.left < leftPaddle.right && aBall.top < leftPaddle.top && aBall.bottom > leftPaddle.bottom && aBall.dirX < 0.0f) { // Left Collison
			Mix_PlayChannel(-1, hit, 0);
			aBall.dirX *= -1.0f;
			aBall.speed *= 1.1f;
		} else if (aBall.right > rightPaddle.left && aBall.top < rightPaddle.top && aBall.bottom > rightPaddle.bottom && aBall.dirX > 0.0f) { // Right Collison
			Mix_PlayChannel(-1, hit, 0);
			aBall.dirX *= -1.0f;
			aBall.speed *= 1.1f;
		}
		aBall.left += aBall.dirX * elapsed * aBall.speed;
		aBall.right += aBall.dirX * elapsed * aBall.speed;
		aBall.top += aBall.dirY * elapsed * aBall.speed;
		aBall.bottom += aBall.dirY * elapsed * aBall.speed;
		ball.Translate(aBall.left, aBall.bottom, 0.0f);
		program.setModelMatrix(ball);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		// End Ball

		// Game End
		if (aBall.left < -3.55f || aBall.right > 3.55f) {
			glBindTexture(GL_TEXTURE_2D, winner);
			modelMatrix.identity();
			program.setModelMatrix(modelMatrix);

			Mix_PlayChannel(-1, yay, 0);

			if (aBall.left > 0) // Left side wins
				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, e1Vertices);
			else if (aBall.left < 0) // Right side wins
				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, e2Vertices);
			glEnableVertexAttribArray(program.positionAttribute);
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program.positionAttribute);
			glDisableVertexAttribArray(program.texCoordAttribute);
			SDL_GL_SwapWindow(displayWindow); // You need this to draw the last image before closing
			Sleep(3000); // I don't know a better way of doing this
			break;
		}
		// End Game End

		SDL_GL_SwapWindow(displayWindow);
		// End Drawing
	}

	SDL_Quit();
	return 0;
}
