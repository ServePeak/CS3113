#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <vector>
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

enum GameState {MAIN_MENU, GAME_PLAY, WIN_SCREEN, LOSE_SCREEN};

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int texID, float u1, float v1, float width1, float height1, float size1) : textureID(texID), u(u1), v(v1), width(width1), height(height1), size(size1) {}

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;

	void SheetSprite::Draw(ShaderProgram* program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size ,
			0.5f * size * aspect, -0.5f * size };
		
		glUseProgram(program->programID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
};

class Vector3 {
public:
	Vector3() {}
	Vector3(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}

	float x;
	float y;
	float z;
};

class Entity {
public:
	Entity() {}
	Entity(float x, float y, float z, float x1, float y1, float z1, SheetSprite sprite1) : position(x,y,z), velocity(x1,y1,z1), sprite(sprite1) {}

	Vector3 position;
	Vector3 velocity;
	Vector3 size;

	float rotation;
	SheetSprite sprite;

};

// Lotsa global game variables
int state = 0;
int hit = 0;
Entity player;
std::vector<Entity> enemies;
std::vector<Entity> pbullet;
std::vector<Entity> ebullet;
SheetSprite playerSprite;
SheetSprite enemySprite;
SheetSprite pbulletSprite;
SheetSprite ebulletSprite;
float lastpbullet = 0.0f;
float lastebullet = 0.0f;
float lastmovedown = 0.0f;

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

void DrawText(ShaderProgram* program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void RenderMenu(Matrix& modelMatrix, ShaderProgram& program) {
	GLuint font = LoadTexture(RESOURCE_FOLDER"Textures/font1.png");
	GLuint invader = LoadTexture(RESOURCE_FOLDER"Textures/menu_invader.png");

	// Invader
	glBindTexture(GL_TEXTURE_2D, invader);
	modelMatrix.identity();
	program.setModelMatrix(modelMatrix);

	float aVertices[] = { -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 2.0f };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
	// End Invader

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-1.0f, 0.0f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "SPACE INVADERS", 0.15f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-1.32f, -0.2f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "LEFT AND RIGHT KEYS TO MOVE", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-0.83f, -0.3f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "SPACEBAR TO SHOOT", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-1.07f, -0.7f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "PRESS SPACEBAR TO PLAY", 0.1f, 0.0f);
}

void RenderLose(Matrix& modelMatrix, ShaderProgram& program) {
	GLuint font = LoadTexture(RESOURCE_FOLDER"Textures/font1.png");
	GLuint invader = LoadTexture(RESOURCE_FOLDER"Textures/menu_invader.png");

	// Invader
	glBindTexture(GL_TEXTURE_2D, invader);
	modelMatrix.identity();
	program.setModelMatrix(modelMatrix);

	float aVertices[] = { -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 2.0f };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
	// End Invader

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-0.5f, 0.0f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "YOU LOSE", 0.15f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-1.2f, -0.5f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "YOU HIT " + std::to_string(hit) + " ENEMIES", 0.15f, 0.0f);
}

void RenderWin(Matrix& modelMatrix, ShaderProgram& program) {
	GLuint font = LoadTexture(RESOURCE_FOLDER"Textures/font1.png");
	GLuint invader = LoadTexture(RESOURCE_FOLDER"Textures/menu_invader.png");

	// Invader
	glBindTexture(GL_TEXTURE_2D, invader);
	modelMatrix.identity();
	program.setModelMatrix(modelMatrix);

	float aVertices[] = { -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 0.0f, 1.0f, 2.0f, -1.0f, 2.0f };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, aVertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
	// End Invader

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-0.5f, 0.0f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "YOU WIN!", 0.15f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.5f, 1.5f, 0.0f);
	modelMatrix.Translate(-2.0f, -0.5f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "PRESS SPACEBAR TO PLAY AGAIN", 0.15f, 0.0f);
}

void RenderGameplay(Matrix& modelMatrix, ShaderProgram& program) {
	// Draw yourself
	modelMatrix.identity();
	modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
	program.setModelMatrix(modelMatrix);
	player.sprite.Draw(&program);

	// Draw enemies
	for (size_t i = 0; i < enemies.size(); i++) {
		modelMatrix.identity();
		modelMatrix.Translate(enemies[i].position.x, enemies[i].position.y, enemies[i].position.z);
		program.setModelMatrix(modelMatrix);
		enemies[i].sprite.Draw(&program);
	}

	// Draw your bullets
	for (size_t i = 0; i < pbullet.size(); i++) {
		modelMatrix.identity();
		modelMatrix.Translate(pbullet[i].position.x, pbullet[i].position.y, pbullet[i].position.z);
		program.setModelMatrix(modelMatrix);
		pbullet[i].sprite.Draw(&program);
	}

	// Draw enemy bullets
	for (size_t i = 0; i < ebullet.size(); i++) {
		modelMatrix.identity();
		modelMatrix.Translate(ebullet[i].position.x, ebullet[i].position.y, ebullet[i].position.z);
		program.setModelMatrix(modelMatrix);
		ebullet[i].sprite.Draw(&program);
	}
}

void UpdateGameplay(Matrix& modelMatrix, ShaderProgram& program, float elapsed) {
	// Enable keyboard
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_LEFT] && player.position.x > -3.55f) {
		if (keys[SDL_SCANCODE_SPACE] && lastpbullet >= 0.5f) {
			lastpbullet = 0.0f;
			pbullet.push_back(Entity(player.position.x, player.position.y, player.position.z, 0.0f, 1.0f, 0.0f, pbulletSprite));
		}
		player.position.x -= player.velocity.x * elapsed;
	} else if (keys[SDL_SCANCODE_RIGHT] && player.position.x < 3.55f) {
		if (keys[SDL_SCANCODE_SPACE] && lastpbullet >= 0.5f) {
			lastpbullet = 0.0f;
			pbullet.push_back(Entity(player.position.x, player.position.y, player.position.z, 0.0f, 1.0f, 0.0f, pbulletSprite));
		}
		player.position.x += player.velocity.x * elapsed;
	} else if (keys[SDL_SCANCODE_SPACE] && lastpbullet >= 0.5f ) {
		lastpbullet = 0.0f;
		pbullet.push_back(Entity(player.position.x, player.position.y, player.position.z, 0.0f, 1.0f, 0.0f, pbulletSprite));
	}

	// Move your bullets
	for (size_t i = 0; i < pbullet.size(); i++) {
		pbullet[i].position.y += pbullet[i].velocity.y * elapsed;
	}

	// Move enemy and check enemy wall collision
	for (size_t i = 0; i < enemies.size(); i++) {
		// To make your life a misery it gets faster as more enemies die
		enemies[i].position.x += enemies[i].velocity.x * elapsed *(36 / pow(enemies.size(), 2) + 1.0f);
		
	}

	for (size_t i = 0; i < enemies.size(); i++) {
		if (lastmovedown > 0.0f && (enemies[i].position.x > 3.55 || enemies[i].position.x < -3.55)) {
			lastmovedown = 0.0f;
			for (size_t j = 0; j < enemies.size(); j++) {
				enemies[j].velocity.x *= -1.0f;
				enemies[j].position.y -= 0.3f;
			}
		}
	}

	// Check enemy vs your bullet collision
	for (size_t i = 0; i < pbullet.size(); i++) {
		for (size_t j = 0; j < enemies.size(); j++) {
			if (pbullet[i].position.x + pbulletSprite.width * 2.0 < enemies[j].position.x + enemySprite.width * 2.0 &&
				pbullet[i].position.x - pbulletSprite.width * 2.0 > enemies[j].position.x - enemySprite.width * 2.0 &&
				pbullet[i].position.y + pbulletSprite.height * 2.0 < enemies[j].position.y + enemySprite.height * 2.0 &&
				pbullet[i].position.y - pbulletSprite.height * 2.0 > enemies[j].position.y - enemySprite.height * 2.0) {
				hit += 1;
				std::swap(pbullet[i], pbullet[pbullet.size() - 1]);
				pbullet.pop_back();
				std::swap(enemies[j], enemies[enemies.size() - 1]);
				enemies.pop_back();
				break;
			}
		}
	}

	// "Randomly" create enemy bullet
	if (lastebullet >= 1.0f) {
		lastebullet = 0.0f;
		int shot = rand() % enemies.size();
		ebullet.push_back(Entity(enemies[shot].position.x, enemies[shot].position.y, enemies[shot].position.z, 0.0f, 1.0f, 0.0f, ebulletSprite));
	}

	// Move enemy bullet
	for (size_t i = 0; i < ebullet.size(); i++) {
		ebullet[i].position.y -= ebullet[i].velocity.y * elapsed;
	}

	// Check yourself vs enemy bullet collision
	for (size_t i = 0; i < ebullet.size(); i++) {
		if (ebullet[i].position.x + ebulletSprite.width * 2.0 < player.position.x + playerSprite.width * 2.0 &&
			ebullet[i].position.x - ebulletSprite.width * 2.0 > player.position.x - playerSprite.width * 2.0 &&
			ebullet[i].position.y + ebulletSprite.height * 2.0 < player.position.y + playerSprite.height * 2.0 &&
			ebullet[i].position.y - ebulletSprite.height * 2.0 > player.position.y - playerSprite.height * 2.0) {
			state = 3;
		}
	}

	// Enemy has reached the front lines! Retreat!
	for (size_t i = 0; i < enemies.size(); i++) {
		if (enemies[i].position.y - enemySprite.height * 2.0 <= player.position.y - playerSprite.height * 2.0) {
			state = 3;
		}
	}

	// We have successfully cleared all visible enemies!
	if (enemies.size() == 0) {
		state = 2;
	}
}

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("You are the invader shooting people defending their home", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	// Setup
	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float lastFrameTicks = 0.0f;

	// Open spritesheet
	GLuint spriteSheetTexture = LoadTexture("Textures/sheet.png");

	// Create player
	playerSprite = SheetSprite(spriteSheetTexture, 586.0f / 1024.0f, 0.0f / 1024.0f, 51.0f / 1024.0f, 75.0f / 1024.0f, 0.2f);
	player = Entity(0.0f, -1.8f, 0.0f, 2.0f, 0.0f, 0.0f, playerSprite);

	// Create enemy
	enemySprite = SheetSprite(spriteSheetTexture, 120.0f / 1024.0f, 520.0f / 1024.0f, 104.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
	for (int i = 0; i < 36; i++) {
		enemies.push_back(Entity(float(-2.5 + (i % 6 * 0.5)), float(1.5 - (i / 6 * 0.3)), 0.0f, 0.5f, 0.0f, 0.0f, enemySprite));
	}

	// Create your bullets
	pbulletSprite = SheetSprite(spriteSheetTexture, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.2f);

	// Create enemy bullets
	ebulletSprite = SheetSprite(spriteSheetTexture, 856.0f / 1024.0f, 926.0f / 1024.0f, 9.0f / 1024.0f, 57.0f / 1024.0f, 0.2f);
	// End Setup

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		// Time
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		lastpbullet += elapsed;
		lastebullet += elapsed;
		lastmovedown += elapsed;

		// Drawing
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		// Enable Keyboard Inputs
		const Uint8* keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_SPACE] && (state == 0 || state == 2)) {
			state = 1;
		}

		// Render state
		switch (state) {
			case MAIN_MENU:
				RenderMenu(modelMatrix, program);
			break;
			case GAME_PLAY:
				RenderGameplay(modelMatrix, program);
			break;
			case LOSE_SCREEN:
				RenderLose(modelMatrix, program);
			break;
			case WIN_SCREEN:
				RenderWin(modelMatrix, program);
			break;
		}

		// Update state
		switch (state) {
			case MAIN_MENU:
				break;
			case GAME_PLAY:
				UpdateGameplay(modelMatrix, program, elapsed);
				break;
			case LOSE_SCREEN:
				break;
			case WIN_SCREEN:
				break;
		}

		SDL_GL_SwapWindow(displayWindow);
		// End Drawing
	}

	SDL_Quit();
	return 0;
}
