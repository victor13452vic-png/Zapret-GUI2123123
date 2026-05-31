@echo off
echo Building Zapret Gui...

if exist "Assets\icon.png" (
    if not exist "Assets\icon.ico" (
        echo Converting icon...
        magick "Assets\icon.png" -define icon:auto-resize=256,128,64,48,32,16 "Assets\icon.ico" 2>nul
        if not exist "Assets\icon.ico" (
            echo Failed to create icon.ico
            echo Please convert Assets\icon.png to Assets\icon.ico manually
            pause
            exit /b 1
        )
    )
)

if not exist "Assets\icon.ico" (
    echo ERROR: Assets\icon.ico not found
    pause
    exit /b 1
)

windres resources.rc -O coff -o resources.res
if %errorlevel% neq 0 (
    echo Resource compilation failed
    pause
    exit /b 1
)

g++ -o "Zapret Gui.exe" src/main.cpp src/gui.cpp resources.res -lgdi32 -lgdiplus -lcomctl32 -lshell32 -ldwmapi -lole32 -luuid -lwinhttp -mwindows -std=c++17 -static -O2
    echo Build failed
    pause
    exit /b 1
)

echo Done
pause