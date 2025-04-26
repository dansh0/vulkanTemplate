@echo off
setlocal

REM Change to the project directory
cd /d C:\Users\dshor\Documents\Github\vulkanTemplate

REM Check if the first argument is "full" to determine build type
if "%1"=="full" (
    echo Running full build...
    echo Cleaning build directory...
    if exist build rmdir /s /q build
    
    echo Running CMake configuration...
    cmake -S . -B build -G "MinGW Makefiles"
    if errorlevel 1 (
        echo CMake configuration failed
        exit /b 1
    )
) else (
    echo Running incremental build...
)

REM Build the project
cmake --build build
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

REM Run the executable
echo Running Obj Viewer...
build\ObjViewer.exe

endlocal 