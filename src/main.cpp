#include "gui.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    ZapretGUI app(hInstance);
    return app.Run(nCmdShow);
}