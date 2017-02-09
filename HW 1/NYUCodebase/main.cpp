#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <stdlib.h>
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

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("good shit thats some good shit right there", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 450, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	// Setup
	glViewport(0, 0, 640, 450);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint texCrying = LoadTexture(RESOURCE_FOLDER"Textures/crying.png");
	GLuint texOk = LoadTexture(RESOURCE_FOLDER"Textures/ok.png");
	GLuint texCheck = LoadTexture(RESOURCE_FOLDER"Textures/check.png");
	GLuint tex100 = LoadTexture(RESOURCE_FOLDER"Textures/100.png");
	GLuint texEyes = LoadTexture(RESOURCE_FOLDER"Textures/eyes.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.5f, 2.5f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0;
	float crying = 0.0;
	float angle = 0.0;
	float eyes = 0.0;
	float check = 0.0;
	float ok = 0.0;
	float one = 0.0;
	// End Setup

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
		angle += elapsed;
		crying += elapsed * (rand() % 20 + 1);
		eyes += elapsed * (rand() % 20 + 1);
		one += elapsed * (rand() % 300 + 1);
		check += elapsed * (rand() % 20 + 1);
		ok += elapsed * (rand() % 300 + 1);

		if (eyes >= 7.0f)
			eyes = -1.0f;
		if (crying >= 7.0f)
			crying = -1.0f;
		if (check >= 7.0f)
			check = -1.0f;

		// Drawing
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		// Crying Face
		glBindTexture(GL_TEXTURE_2D, texCrying);
		modelMatrix.identity();
		modelMatrix.Translate(crying, 0.0f, 0.0f);
		//modelMatrix.Rotate(crying * (3.1415926 / 180.0));
		program.setModelMatrix(modelMatrix);

		float aVertices[] = {-3.5, -0.5, -2.5, -0.5, -2.5, 0.5, -3.5, -0.5, -2.5, 0.5, -3.5, 0.5};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float aTexCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, aTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Crying Face

		// 100
		glBindTexture(GL_TEXTURE_2D, tex100);
		modelMatrix.identity();
		//modelMatrix.Translate(one, 0.0f, 0.0f);
		modelMatrix.Rotate(one * (3.1415926 / 180.0));
		program.setModelMatrix(modelMatrix);

		float bVertices[] = { -3.5, -0.5, -2.5, -0.5, -2.5, 0.5, -3.5, -0.5, -2.5, 0.5, -3.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float bTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, bTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End 100

		// Check
		glBindTexture(GL_TEXTURE_2D, texCheck);
		modelMatrix.identity();
		modelMatrix.Translate(check, 0.0f, 0.0f);
		//modelMatrix.Rotate(angle * 30.0 * (3.1415926 / 180.0));
		program.setModelMatrix(modelMatrix);

		float cVertices[] = { -3.5, 1.5, -2.5, 1.5, -2.5, 2.5, -3.5, 1.5, -2.5, 2.5, -3.5, 2.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float cTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, cTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Check

		// OK
		glBindTexture(GL_TEXTURE_2D, texOk);
		modelMatrix.identity();
		//modelMatrix.Translate(ok, 0.0f, 0.0f);
		modelMatrix.Rotate(ok * (-3.1415926 / 180.0));
		program.setModelMatrix(modelMatrix);

		float dVertices[] = { -2.0, -0.5, -1.0, -0.5, -1.0, 0.5, -2.0, -0.5, -1.0, 0.5, -2.0, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float dTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, dTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End OK

		// Eyes
		glBindTexture(GL_TEXTURE_2D, texEyes);
		modelMatrix.identity();
		modelMatrix.Translate(eyes, 0.0f, 0.0f);
		//modelMatrix.Rotate(angle * 30.0 * (3.1415926 / 180.0));
		program.setModelMatrix(modelMatrix);

		float eVertices[] = { -3.5, -2.5, -2.5, -2.5, -2.5, -1.5, -3.5, -2.5, -2.5, -1.5, -3.5, -1.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, eVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float eTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, eTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		// End Eyes

		SDL_GL_SwapWindow(displayWindow);
		// End Drawing
	}

	SDL_Quit();
	return 0;
}
