@ECHO off
CD C:\Users\User\Documents\GitHub\Raycasting-Renderer\src


REM Delete all .o files and force recompile.
IF EXIST "constants.o" (
    DEL /Q "constants.o"
)
IF EXIST "physics.o" (
    DEL /Q "physics.o"
)
IF EXIST "raycasting.o" (
    DEL /Q "raycasting.o"
)
IF EXIST "render.o" (
    DEL /Q "render.o"
)
IF EXIST "utils.o" (
    DEL /Q "utils.o"
)
IF EXIST "global.o" (
    DEL /Q "global.o"
)


CD C:\Users\User\Documents\GitHub\Raycasting-Renderer
REM Compile.
mingw32-make
REM Wait for user, then start app.exe.
PAUSE
start app.exe