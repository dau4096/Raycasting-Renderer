extern "C" {
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/display.h"
    #include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/keyboard.h"
	#include "C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6\include\fxcg/rtc.h"
}
#include "utils.h"
using namespace std;


namespace utils {


unsigned short createColour(unsigned char r, unsigned char g, unsigned char b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}



void drawPixel(int x, int y, unsigned short colour) {
    if (x >= 0 && x < LCD_WIDTH_PX && y >= 0 && y < LCD_HEIGHT_PX) {
        Bdisp_SetPoint_VRAM(x, y+24, colour);
    }
}


void drawLine(int x, int height, unsigned short colour) {
	unsigned short finalColour;
	for (int yOffset = 0; yOffset < LCD_HEIGHT_PX; yOffset++) {
		if (yOffset > (LCD_HEIGHT_PX+height)/2) {
			finalColour = 33808; //GroundColour (128, 128, 128)
		} else if (yOffset <= (LCD_HEIGHT_PX-height)/2) {
			finalColour = 34429; //SkyColour (135, 206, 235)
		} else {
			finalColour = colour; //Wall Colour
		}
		drawPixel(x, yOffset, finalColour);
	}
}


float sqrtApprox(float value) {
    float guess = value * 0.5f;  //x0 = x/2
    const float epsilon = 0.01f; //1-hundreths of precision. Should be accurate enough.

    //Iterate until the guess is sufficiently accurate
    while (true) {
        float newGuess = 0.5f * (guess + value / guess);  //Update guess using Newton's method
        if (utils::abs(newGuess - guess) < epsilon) {
            break;  //Stop when the guess is sufficiently accurate
        }
        guess = newGuess;
    }

    return guess;
}

float dot(vec2 A, vec2 B) {
	return A.x * B.x + A.y * B.y;
}

float determinant(vec2 A, vec2 B) {
	return A.x * B.y - A.y * B.x;
}

float length(vec2 vector) {
	return sqrtApprox((vector.x * vector.x) + (vector.y * vector.y));
}

vec2 normalise(vec2 vector) {
	float len = length(vector);
	if (len == 0) return vec2(0, 0);
	return DIV(vector, len);
}

vec2 ADD(vec2 A, vec2 B) {
	return vec2(A.x + B.x, A.y + B.y);
}

vec2 SUB(vec2 A, vec2 B) {
	return vec2(A.x - B.x, A.y - B.y);
}

vec2 MUL(vec2 A, float scalar) {
	return vec2(A.x * scalar, A.y * scalar);
}

vec2 DIV(vec2 A, float scalar) {
	return vec2(A.x / scalar, A.y / scalar);
}


float min(float A, float B) {
	if (A < B) return A;
	return B;
}
float max(float A, float B) {
	if (A < B) return B;
	return A;
}

float clamp(float value, float minV, float maxV) {
	return max(minV, min(value, maxV));
}

float angleClamp(float angle) {
	while (angle < -180.0f) angle += 360.0f;
	while (angle > 180.0f) angle -= 360.0f;
    return angle;
}

float abs(float value) {
	if (value < 0.0f) return -value;
	return value;
}

int scaleAngleInt(float angle) {
    int scaledAngle = static_cast<int>((angle + 2.5f) / 5);
    return (scaledAngle % 72 + 72) % 72;
}

float sin(float angle) {
	int scaledAngle = scaleAngleInt(angle);
	return sinLUT[scaledAngle];
}

float cos(float angle) {
	int scaledAngle = scaleAngleInt(angle);
	return cosLUT[scaledAngle];	
}

}