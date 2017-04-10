#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
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

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

// It's all the same
float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

class Vector3 {
public:
	Vector3() {}
	Vector3(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}

	float x;
	float y;
	float z;

	float length() const {
		return sqrt(x*x + y*y + z*z);
	}

	void normalize() {
		x = x / length();
		y = y / length();
		z = z / length();
	}

	Vector3 operator*(const Matrix& v) {
		Vector3 v1;
		v1.x = v.m[0][0] * x + v.m[1][0] * y + v.m[2][0] * z + v.m[3][0] * 1;
		v1.y = v.m[0][1] * x + v.m[1][1] * y + v.m[2][1] * z + v.m[3][1] * 1;
		v1.z = v.m[0][2] * x + v.m[1][2] * y + v.m[2][2] * z + v.m[3][2] * 1;
		return v1;
	}
};

// It angers me that this function is inbetween classes
bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector3> &points1, const std::vector<Vector3> &points2, Vector3 &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector3 &p1, const Vector3 &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector3> &e1Points, const std::vector<Vector3> &e2Points, Vector3 &penetration) {
	std::vector<Vector3> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector3 e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector3 e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector3 ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
}

class Entity {
public:
	Entity() {}
	Entity(float x, float y, float z, float x1, float y1, float z1, float x2, float y2, float z2, float r, GLuint t1) : position(x,y,z), velocity(x1,y1,z1), scale(x2,y2,z2), rotation(r), texture(t1) {}

	Vector3 position;
	Vector3 velocity;
	Vector3 scale;
	Vector3 penetration;
	float rotation;

	std::vector<Vector3> points = std::vector<Vector3>(6);
	std::vector<Vector3> vertices = std::vector<Vector3>(6);

	Matrix matrix;
	GLuint texture;

	void render(ShaderProgram *program) {
		matrix.identity();
		matrix.Translate(position.x, position.y, position.z);
		matrix.Rotate(rotation * (3.1415926f / 180.0f));
		matrix.Scale(scale.x, scale.y, scale.z);
		program->setModelMatrix(matrix);

		float vert[] = { vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y, vertices[3].x, vertices[3].y, vertices[4].x, vertices[4].y, vertices[5].x, vertices[5].y };

		glBindTexture(GL_TEXTURE_2D, texture);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vert);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	void update(float elapsed) {
		rotation += 10.0 * elapsed;
		if (position.x >= 3.55f || position.x <= -3.55f)
			velocity.x *= -1.0;
		position.x += velocity.x * elapsed;
		if (position.y >= 2.0f || position.y <= -2.0f)
			velocity.y *= -1.0;
		position.y += velocity.y * elapsed;
		// Rotated points 
		for (int i = 0; i < points.size(); i++) {
			points[i] = vertices[i] * matrix;
		}
	}

	// I tried to do this without it in entities and it didn't work :(
	void collision(Entity& other) {
		if (checkSATCollision(points, other.points, penetration)) {
			other.position.x -= penetration.x / 1.5f;
			other.position.y -= penetration.y / 1.5f;
			velocity.x *= -1.0f;
			velocity.y *= -1.0f;
			other.velocity.x *= -1.0f;
			other.velocity.y *= -1.0f;
		}

	}
};

std::vector<Entity> entities;

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

// I understand why it's easier to render and update within entities now. Trying to do this without that was a horror show.
void RenderGameplay(Matrix& modelMatrix, ShaderProgram* program) {
	for (size_t i = 0; i < entities.size(); i++) {
		entities[i].render(program);
	}
}

void UpdateGameplay(float elapsed) {
	for (size_t i = 0; i < entities.size(); i++) {
		entities[i].update(elapsed);
		for (size_t j = i + 1; j < entities.size(); j++) {
			entities[i].collision(entities[j]);
		}
	}
}

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("C O L L I S I O N D E M O", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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

	// Load textures
	GLuint beige = LoadTexture(RESOURCE_FOLDER"Textures/alienBeige_square.png");
	GLuint blue = LoadTexture(RESOURCE_FOLDER"Textures/alienBlue_square.png");
	GLuint yellow = LoadTexture(RESOURCE_FOLDER"Textures/alienYellow_square.png");

	// Creating entities
	entities.push_back(Entity(0.0f, -0.0f, 0.0f, 0.2f, 0.2f, 0.0f, 3.0f, 3.0f, 2.0f, 0.0f, beige));
	entities.push_back(Entity(1.5f, 1.0f, 0.0f, 0.4f, 0.4f, 0.0f, 4.0f, 4.0f, 1.0f, 45.0f, blue));
	entities.push_back(Entity(-1.5f, 1.0f, 0.0f, 0.8f, 0.8f, 0.0f, 1.5f, 1.5f, 1.5f, 130.0f, yellow));

	// Some disgusting code here, don't mind me.
	for (size_t i = 0; i < entities.size(); i++) {
		entities[i].vertices[0] = Vector3(-0.1, -0.1, 0.1); 
		entities[i].vertices[1] = Vector3(0.1, -0.1, 0.1); 
		entities[i].vertices[2] = Vector3(0.1, 0.1, 0.1);
		entities[i].vertices[3] = Vector3(-0.1, -0.1, 0.1); 
		entities[i].vertices[4] = Vector3(0.1, 0.1, 0.1);
		entities[i].vertices[5] = Vector3(-0.1, 0.1, 0.1);
	}

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
		RenderGameplay(modelMatrix, &program);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
