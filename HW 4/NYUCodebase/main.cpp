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

enum EntityType {ENTITY_PLAYER, ENTITY_ENEMY, ENTITY_COIN};
// 60 FPS (1.0f/60.0f)
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

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
	Entity(float x, float y, float z, float x1, float y1, float z1, float x2, float y2, float z2, bool stat, SheetSprite sprite1) : position(x,y,z), velocity(x1,y1,z1), acceleration(x2,y2,z2), isStatic(stat), sprite(sprite1) {}

	Vector3 position;
	Vector3 size;
	Vector3 velocity;
	Vector3 acceleration;

	SheetSprite sprite;

	bool isStatic;
	bool collideUp;
	bool collideDown;
	bool collideLeft;
	bool collideRight;

	EntityType entityType;
};

// Lotsa global game variables
std::vector<Entity> entities;
Entity player;
Entity star;
SheetSprite playerSprite;
SheetSprite starSprite;
SheetSprite entitySprite;
float gravity = 0.5f;
int cvalue = 0;

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

void RenderGameplay(Matrix& modelMatrix, ShaderProgram& program) {
	// Coin text
	GLuint font = LoadTexture(RESOURCE_FOLDER"Textures/font1.png");
	modelMatrix.identity();
	modelMatrix.Scale(1.0f, 1.0f, 0.0f);
	modelMatrix.Translate(-3.50f, 1.9f, 0.0f);
	program.setModelMatrix(modelMatrix);
	DrawText(&program, font, "Coins: " + std::to_string(cvalue), 0.15f, 0.0f);

	// Draw yourself
	modelMatrix.identity();
	modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
	program.setModelMatrix(modelMatrix);
	player.sprite.Draw(&program);

	// Draw all entities
	for (size_t i = 0; i < entities.size(); i++) {
		modelMatrix.identity();
		modelMatrix.Translate(entities[i].position.x, entities[i].position.y, entities[i].position.z);
		program.setModelMatrix(modelMatrix);
		entities[i].sprite.Draw(&program);
	}
}

void setCollideFalse(Entity& entity) {
	entity.collideUp = false;
	entity.collideDown = false;
	entity.collideLeft = false;
	entity.collideRight = false;
}

void UpdateGameplay(float elapsed) {
	// Enable keyboard
	const Uint8* keys = SDL_GetKeyboardState(NULL);

	setCollideFalse(player);

	// Player collisions
	for (size_t i = 0; i < entities.size(); i++) {
		if (player.position.x - playerSprite.width * 4 < entities[i].position.x + entitySprite.width * 2 &&
			player.position.x + playerSprite.width * 4 > entities[i].position.x - entitySprite.width * 2 &&
			player.position.y - playerSprite.height * 2 < entities[i].position.y + entitySprite.height * 2 &&
			player.position.y + playerSprite.height * 2 > entities[i].position.y - entitySprite.height * 2) {
			if (entities[i].isStatic) { // Static collision
				if (player.position.y > entities[i].position.y)
					player.collideDown = true;
				else if (player.position.y < entities[i].position.y)
					player.collideUp = true;
				if (player.position.x > entities[i].position.x + entitySprite.width * 2)
					player.collideRight = true;
				else if (player.position.x < entities[i].position.x - entitySprite.width * 2)
					player.collideLeft = true;
			} else if (entities[i].entityType = ENTITY_COIN) { // Dynamic collision for coins
				std::swap(entities[i], entities[entities.size()-1]);
				entities.pop_back();
				cvalue++;
			}
		}
	}
	
	// Stop y axis movement if colliding
	if ((player.collideDown && player.velocity.y < 0.0f) || (player.collideUp && player.velocity.y > 0.0f))
		player.velocity.y = 0.0f;
	else { // or keep doing what you were doing
		for (size_t i = 0; i < entities.size(); i++)
			entities[i].position.y -= player.velocity.y * elapsed;
		player.velocity.y -= gravity * elapsed;
	}
	if (keys[SDL_SCANCODE_LEFT]) {
		if (!player.collideRight) { // Pressing left moves the entities right!
			for (size_t i = 0; i < entities.size(); i++)
				entities[i].position.x += player.velocity.x * elapsed;
		}
	} else if (keys[SDL_SCANCODE_RIGHT]) {
		if (!player.collideLeft) { // Pressing right moves the entities left!
			for (size_t i = 0; i < entities.size(); i++)
				entities[i].position.x -= player.velocity.x * elapsed;
		}
	}
	// Jump
	if (keys[SDL_SCANCODE_UP] && player.collideDown == true)
		player.velocity.y = 1.0f;
}

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Single coin platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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
	player = Entity(0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, false, playerSprite);
	player.entityType = ENTITY_PLAYER;

	// Create star
	starSprite = SheetSprite(spriteSheetTexture, 778.0f / 1024.0f, 557.0f / 1024.0f, 31.0f / 1024.0f, 30.0f / 1024.0f, 0.2f);
	star = Entity(0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, false, starSprite);
	star.entityType = ENTITY_COIN;

	// Create entities
	entitySprite = SheetSprite(spriteSheetTexture, 0.0f / 1024.0f, 78.0f / 1024.0f, 222.0f / 1024.0f, 39.0f / 1024.0f, 0.2f);
	entities.push_back(Entity(0.0f, -2.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, true, entitySprite));
	entities.push_back(Entity(1.0f, -1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, true, entitySprite));
	entities.push_back(Entity(-1.0f, -1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, true, entitySprite));
	entities.push_back(Entity(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, true, entitySprite));
	entities.push_back(star);

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
		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}
		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			UpdateGameplay(FIXED_TIMESTEP);
		}
		UpdateGameplay(fixedElapsed);

		// Drawing
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		// Enable Keyboard Inputs
		const Uint8* keys = SDL_GetKeyboardState(NULL);

		UpdateGameplay(elapsed);
		RenderGameplay(modelMatrix, program);

		SDL_GL_SwapWindow(displayWindow);
		// End Drawing
	}

	SDL_Quit();
	return 0;
}
