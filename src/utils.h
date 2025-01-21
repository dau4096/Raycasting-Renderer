#ifndef UTILS_H
#define UTILS_H


struct vec2 {
	float x, y;

	constexpr vec2() : x(0.0f), y(0.0f) {}
	constexpr vec2(float x, float y)
		: x(x), y(y) {}
};

struct vec3 {
	float x, y, z;

	constexpr vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	constexpr vec3(float x, float y, float z)
		: x(x), y(y), z(z) {}
};

struct Wall {
	vec2 start, end;
	bool valid;

    Wall() : start(0.0f, 0.0f), end(0.0f, 0.0f), valid(false) {}

	Wall(vec2 start, vec2 end)
		: start(start), end(end), valid(true) {}
};

struct Sprite {
	vec2 position;
	float width;
	bool valid;

	Sprite() : position(0.0f, 0.0f), width(0.0f), valid(false) {}

	Sprite(vec2 position, float width, int textureID)
		: position(position), width(width), valid(true) {}
};

struct Ray {
	vec2 position, direction, end;

	Ray(vec2 position, vec2 direction)
		: position(position), direction(direction) {}
};

struct Player {
	vec2 position;
	float viewAngle;

	Player(vec2 position, float angle)
		: position(position), viewAngle(angle) {}
};


#define RGB565(r, g, b) ((r & 0x1F) << 11 | (g & 0x3F) << 5 | (b & 0x1F));



constexpr float sinLUT[72] = {0.0, 0.0872, 0.1736, 0.2588, 0.342, 0.4226, 0.5, 0.5736, 0.6428, 0.7071, 0.766, 0.8192, 0.866, 0.9063, 0.9397, 0.9659, 0.9848, 0.9962, 1.0, 0.9962, 0.9848, 0.9659, 0.9397, 0.9063, 0.866, 0.8192, 0.766, 0.7071, 0.6428, 0.5736, 0.5, 0.4226, 0.342, 0.2588, 0.1736, 0.0872, 0.0, -0.0872, -0.1736, -0.2588, -0.342, -0.4226, -0.5, -0.5736, -0.6428, -0.7071, -0.766, -0.8192, -0.866, -0.9063, -0.9397, -0.9659, -0.9848, -0.9962, -1.0, -0.9962, -0.9848, -0.9659, -0.9397, -0.9063, -0.866, -0.8192, -0.766, -0.7071, -0.6428, -0.5736, -0.5, -0.4226, -0.342, -0.2588, -0.1736, -0.0872};
constexpr float cosLUT[72] = {1.0, 0.9962, 0.9848, 0.9659, 0.9397, 0.9063, 0.866, 0.8192, 0.766, 0.7071, 0.6428, 0.5736, 0.5, 0.4226, 0.342, 0.2588, 0.1736, 0.0872, 0.0, -0.0872, -0.1736, -0.2588, -0.342, -0.4226, -0.5, -0.5736, -0.6428, -0.7071, -0.766, -0.8192, -0.866, -0.9063, -0.9397, -0.9659, -0.9848, -0.9962, -1.0, -0.9962, -0.9848, -0.9659, -0.9397, -0.9063, -0.866, -0.8192, -0.766, -0.7071, -0.6428, -0.5736, -0.5, -0.4226, -0.342, -0.2588, -0.1736, -0.0872, -0.0, 0.0872, 0.1736, 0.2588, 0.342, 0.4226, 0.5, 0.5736, 0.6428, 0.7071, 0.766, 0.8192, 0.866, 0.9063, 0.9397, 0.9659, 0.9848, 0.9962};
constexpr vec2 INVALID = vec2(4294967295, 4294967295);


//Utility functions
namespace utils {

	//CASIO SDK Specific;
	void drawPixel(int x, int y, unsigned short colour);
	void drawLine(int x, int height, unsigned short colour);
	unsigned short createColour(unsigned char r, unsigned char g, unsigned char b);


	//Vector operations
	float dot(vec2 A, vec2 B);
	float determinant(vec2 A, vec2 B);
	float length(vec2 vector);
	vec2 normalise(vec2 vector);
	vec2 ADD(vec2 A, vec2 B);
	vec2 SUB(vec2 A, vec2 B);
	vec2 MUL(vec2 A, float scalar);
	vec2 DIV(vec2 A, float scalar);


	//Maths and Trig
	float min(float A, float B);
	float max(float A, float B);
	float clamp(float value, float min, float max);
	float angleClamp(float value);
	float abs(float value);
	float sin(float angle);
	float cos(float angle);
}

#endif