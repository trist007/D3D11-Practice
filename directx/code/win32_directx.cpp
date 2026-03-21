#define WIN32_LEAN_AND_MEAN
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(69);
        } break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CALLBACK WinMain(
                     HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    const auto pClassName = "directx";
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = pClassName;
    wc.hIconSm = 0;
    
    RegisterClassEx(&wc);
    
    // Create window
    HWND hwnd = CreateWindowEx(
                               0, pClassName,
                               "DirectX Tutorial",
                               WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
                               200, 200, 640, 480,
                               0, 0, hInstance, 0
                               );
    
    ShowWindow(hwnd, SW_SHOW);
    
    // Message pump return value for GetMessage is 0 if WM_QUIT is received
    MSG msg;
    BOOL gResult;
    while((gResult = GetMessage(&msg, 0, 0, 0)) > 0 )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if(gResult == -1)
        return(-1);
    else
        return(msg.wParam);
    
}