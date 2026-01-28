@ECHO off
CD src


REM Delete all .o files and force recompile.
IF EXIST "constants.o" (
    DEL /Q "constants.o"
)
IF EXIST "physics.o" (
    DEL /Q "physics.o"
)
IF EXIST "graphics.o" (
    DEL /Q "graphics.o"
)
IF EXIST "utils.o" (
    DEL /Q "utils.o"
)
IF EXIST "loader.o" (
    DEL /Q "loader.o"
)


CD ..
IF EXIST "main.o" (
    DEL /Q "main.o"
)

REM Compile.
mingw32-make
REM Wait for user, then start app.exe.
PAUSE
start app.exe
IF %ERROR_LEVEL% NEQ 0 (
    ECHO An error occurred; %ERROR_LEVEL% &Exit /b 1
    PAUSE
)

