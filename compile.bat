@ECHO off

REM Set SDK path
SET SDK_PATH=C:\Users\User\Documents\code\.cpp\PrizmSDK-win-0.6
SET PATH=%SDK_PATH%\bin;%PATH%

REM Navigate to the project directory
CD C:\Users\User\Documents\GitHub\Raycasting-Renderer

REM Clean up old build artifacts
IF EXIST "src\physics.o" DEL /Q "src\physics.o"
IF EXIST "src\raycasting.o" DEL /Q "src\raycasting.o"
IF EXIST "src\render.o" DEL /Q "src\render.o"
IF EXIST "src\utils.o" DEL /Q "src\utils.o"
IF EXIST "simple_program.elf" DEL /Q "simple_program.elf"
IF EXIST "simple_program.bin" DEL /Q "simple_program.bin"
IF EXIST "simple_program.g3a" DEL /Q "simple_program.g3a"

REM Run the Prizm SDK build process with Prizm's make (from SDK)
%SDK_PATH%\bin\make
PAUSE
