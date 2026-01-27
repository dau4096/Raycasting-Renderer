#ifndef INCLUDES_H
#define INCLUDES_H


//OS Specific
#ifdef _WIN32
//Include Windows.
#include <Windows.h> //Only for windows systems (obviously). Only needed for console-specific functions, which are minimal.
#elif defined(__linux__)
//Include Linux things.
#include <unistd.h>		//Both required for the console output mode.
#include <sys/ioctl.h>  // ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
#endif




//Include GLEW.
#include <GL/glew.h>

//Include OpenGL.
#include <GL/gl.h>
#include <GL/glu.h>

//Include GLM.
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Include GLFW.
#include <GLFW/glfw3.h>

//Include PugiXML
#include <pugixml.hpp>

//Include std subheaders.
#include <clocale>
#include <bits/stdc++.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <ctime>
#include <vector>
#include <array>
#include <set>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <signal.h>

#endif