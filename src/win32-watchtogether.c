/*
TODO:

Add pixel buffer and display it
Give main code access to pixel buffer
Add thread to process streamed data

*/

#define UNICODE
#define _UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "version.h"

static TCHAR win_class_title[] = _T("watchtogether");
//static TCHAR window_title[] = _T("WatchTogether " WT_FULL_VERSION);

typedef struct pixel_buffers {
    internal void* pixel_buffer[2];
}

pixel_buffers buffers;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK wWinMain(  
HINSTANCE hInstance,  
HINSTANCE hPrevInstance,  
LPWSTR     lpCmdLine,  
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
    window_class.hIconSm        = NULL;  
    
    //registering window class
    if(!RegisterClassEx(&window_class))
    {
        MessageBoxW(NULL, _T("RegisterClassEx failed."), WT_WINDOW_TITLE, 0);
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
        MessageBoxW(NULL, _T("CreateWindow() failed."), WT_WINDOW_TITLE, 0);
        return 1;
    }
    
    ShowWindow(window_handle, nCmdShow);
    UpdateWindow(window_handle);
    
    MSG msg;  
    while (GetMessage(&msg, NULL, 0, 0))  
    {  
        TranslateMessage(&msg);  
        DispatchMessage(&msg);  
    } 
    
    return 0;
}


LRESULT CALLBACK WndProc(  
HWND   hwnd,  
UINT   msg,  
WPARAM wParam,  
LPARAM lParam  
)
{
    PAINTSTRUCT ps;
    HDC hdc;
    
    switch(msg)
    {
        case WM_SIZE:
        {
            fprintf(stderr, "WM_SIZE\n");
        }
        break;
        case WM_PAINT:
        {
            fprintf(stderr, "WM_PAINT\n");
            hdc = BeginPaint(hwnd, &ps);
            
            // lay out UI
            
            EndPaint(hwnd, &ps);
        }
        break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
        default: 
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        break;
    }
    
    return 0;
}