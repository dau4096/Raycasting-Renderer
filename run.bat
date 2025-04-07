@ECHO off
CD C:\Users\User\Documents\GitHub\Raycasting-Renderer\src


REM Delete all .o files and force recompile.
IF EXIST "constants.o" (
    DEL /Q "constants.o"
)
IF EXIST "physics.o" (
    DEL /Q "physics.o"
)
IF EXIST "render.o" (
    DEL /Q "render.o"
)
IF EXIST "utils.o" (
    DEL /Q "utils.o"
)


CD C:\Users\User\Documents\GitHub\Raycasting-Renderer
IF EXIST "main.o" (
    DEL /Q "main.o"
)

REM Compile.
mingw32-make
REM Wait for user, then start app.exe.
PAUSE
start app.exe