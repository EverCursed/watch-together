#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "version.h"

static TCHAR win_class_title[] = "watchtogether";
//static TCHAR window_title[] = _T("WatchTogether " WT_FULL_VERSION);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(  
HINSTANCE hInstance,  
HINSTANCE hPrevInstance,  
LPSTR     lpCmdLine,  
int       nCmdShow  
)
{
    WNDCLASSEX window_class;
    
    window_class.cbSize         = sizeof(WNDCLASSEX);
    window_class.style          = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc    = WndProc;
    window_class.cbClsExtra     = 0;  
    window_class.cbWndExtra     = 0;  
    window_class.hInstance      = hInstance;  
    window_class.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));  
    window_class.hCursor        = LoadCursor(NULL, IDC_ARROW);  
    window_class.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);  
    window_class.lpszMenuName   = NULL;  
    window_class.lpszClassName  = win_class_title;  
    window_class.hIconSm        = LoadIcon(window_class.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));  
    
    //registering window class
    if(!RegisterClassEx(&window_class))
    {
        MessageBox(NULL, "RegisterClassEx failed.", WT_WINDOW_TITLE, 0);
        return 1;
    }
    
    HWND window_handle = CreateWindow(
        win_class_title,
        WT_WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        // TODO(Val): Get window size from ini
        1280, 720,
        NULL, 
        NULL,
        hInstance,
        NULL
        );
    
    if(!window_handle)
    {
        MessageBox(NULL, "CreateWindow() failed.", WT_WINDOW_TITLE, 0);
        return 1;
    }
    
    ShowWindow(window_handle, nCmdShow);
    UpdateWindow(window_handle);
    
    return 0;
}


LRESULT CALLBACK WndProc(  
HWND   hwnd,  
UINT   uMsg,  
WPARAM wParam,  
LPARAM lParam  
)
{
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int) msg.wParam;
}
