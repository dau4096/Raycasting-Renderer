CC = g++
CFLAGS = -std=c++23 -O2 -ffast-math -m64 \
         -IC:/Users/User/Documents/code/.cpp/glew-2.1.0/include \
         -IC:/Users/User/Documents/code/.cpp/glm \
         -IC:/Users/User/Documents/code/.cpp/glfw-3.4.bin.WIN64/include \
         -IC:/Users/User/Documents/code/.cpp
LIBS = -LC:/Users/User/Documents/code/.cpp/glew-2.1.0/lib/Release/x64 \
       -LC:/Users/User/Documents/code/.cpp/glfw-3.4.bin.WIN64/lib-mingw-w64 \
       -static-libgcc -static-libstdc++ -Wl,-Bstatic -Wl,-Bdynamic -lglfw3 -lglew32 -lopengl32 -lgdi32 -lwinpthread
SOURCES = main.cpp src/graphics.cpp src/physics.cpp src/utils.cpp src/loader.cpp C:/Users/User/Documents/code/.cpp/pugixml/pugixml.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: app

app: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o app

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del $(OBJECTS) app.exe
