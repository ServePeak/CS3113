#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
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
	displayWindow = SDL_CreateWindow("Pong but with a bigger filesize.", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	// Settop
	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint bg = LoadTexture(RESOURCE_FOLDER"Textures/background.png");
	GLuint dot = LoadTexture(RESOURCE_FOLDER"Textures/dot.png");
	GLuint winner = LoadTexture(RESOURCE_FOLDER"Textures/winner.png");

	Matrix ball;
	Matrix lPaddle;
	Matrix rPaddle;
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	float angle = float((rand() % 4) * 90 + 45);
	float dirX = cos((angle * 3.1415) / 180);
	float dirY = sin((angle * 3.1415) / 180);
	Ball aBall(0.0, 0.14, 0.14, 0.0, dirX, dirY, 1.0);
	Paddle leftPaddle(-3.55, 0.56, -3.41, 0.0);
	Paddle rightPaddle(3.41, 0.56, 3.55, 0.0);

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0;
	float move = 0.0;
	// End Settop

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
		move += elapsed;

		// Drawing
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		// Movement
		const Uint8* keys = SDL_GetKeyboardState(NULL);

		// Background
		glBindTexture(GL_TEXTURE_2D, bg);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		float aVertices[] = { -3.55, -2.0, 3.55, -2.0, 3.55, 2.0, -3.55, -2.0, 3.55, 2.0, -3.55, 2.0 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float aTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, aTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Background

		glBindTexture(GL_TEXTURE_2D, dot); // Make everything from here on white

		// Left Paddle
		lPaddle.identity();
		if (keys[SDL_SCANCODE_W] && leftPaddle.top < 1.86) {
			leftPaddle.top += 0.002;
			leftPaddle.bottom += 0.002;
		}
		else if (keys[SDL_SCANCODE_S] && leftPaddle.bottom > -2.0) {
			leftPaddle.top -= 0.002;
			leftPaddle.bottom -= 0.002;
		}
		lPaddle.Translate(0.0, leftPaddle.bottom, 0.0);
		program.setModelMatrix(lPaddle);

		float bVertices[] = { -3.55, 0.0, -3.41, 0.0, -3.41, 0.56, -3.55, 0.0, -3.41, 0.56, -3.55, 0.56 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float bTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, bTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Left Paddle

		// Right Paddle
		rPaddle.identity();
		if (keys[SDL_SCANCODE_UP] && rightPaddle.top < 1.86) {
			rightPaddle.top += 0.002;
			rightPaddle.bottom += 0.002;
		}
		else if (keys[SDL_SCANCODE_DOWN] && rightPaddle.bottom > -2.0) {
			rightPaddle.top -= 0.002;
			rightPaddle.bottom -= 0.002;
		}
		rPaddle.Translate(0.0, rightPaddle.bottom, 0.0);
		program.setModelMatrix(rPaddle);

		float cVertices[] = { 3.41, 0.0, 3.55, 0.0, 3.55, 0.56, 3.41, 0.0, 3.55, 0.56, 3.41, 0.56 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float cTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, cTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Right Paddle

		// Ball
		ball.identity();
		if (aBall.top > 1.86 && aBall.dirY > 0 || aBall.bottom < -2.0 && aBall.dirY < 0) {
			aBall.dirY *= -1.0;
		} else if (aBall.left < leftPaddle.right && aBall.top < leftPaddle.top && aBall.bottom > leftPaddle.bottom && aBall.dirX < 0) {
			aBall.dirX *= -1.0;
			aBall.speed += 0.1;
		} else if (aBall.right > rightPaddle.left && aBall.top < rightPaddle.top && aBall.bottom > rightPaddle.bottom && aBall.dirX > 0) {
			aBall.dirX *= -1.0;
			aBall.speed += 0.1;
		}
		aBall.left += aBall.dirX * elapsed * aBall.speed;
		aBall.right += aBall.dirX * elapsed * aBall.speed;
		aBall.top += aBall.dirY * elapsed * aBall.speed;
		aBall.bottom += aBall.dirY * elapsed * aBall.speed;
		ball.Translate(aBall.left, aBall.bottom, 0.0);
		program.setModelMatrix(ball);

		float dVertices[] = { 0.0, 0.0, 0.14, 0.0, 0.14, 0.14, 0.0, 0.0, 0.14, 0.14, 0.0, 0.14 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float dTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, dTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Ball

		// Game End
		if (aBall.left < -3.55 || aBall.right > 3.55) {
			glBindTexture(GL_TEXTURE_2D, winner);
			modelMatrix.identity();
			program.setModelMatrix(modelMatrix);

			float e1Vertices[] = { -3.0, -1.0, -1.0, -1.0, -1.0, 1.0, -3.0, -1.0, -1.0, 1.0, -3.0, 1.0 };
			float e2Vertices[] = { 1.0, -1.0, 3.0, -1.0, 3.0, 1.0, 1.0, -1.0, 3.0, 1.0, 1.0, 1.0 };
			if (aBall.left < 0)
				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, e1Vertices);
			else if(aBall.left > 0)
				glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, e2Vertices);
			glEnableVertexAttribArray(program.positionAttribute);

			float eTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, eTexCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program.positionAttribute);
			glDisableVertexAttribArray(program.texCoordAttribute);
			SDL_GL_SwapWindow(displayWindow);
			Sleep(3000); // I don't know a better way of doing this
			break;
		}

		SDL_GL_SwapWindow(displayWindow);
		// End Drawing
	}

	SDL_Quit();
	return 0;
}
