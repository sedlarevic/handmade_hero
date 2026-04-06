#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message, WPARAM WParam,
                                    LPARAM LParam)
{

  LRESULT Result = 0;

  switch (Message)
  {
    case WM_CREATE:
    {
      OutputDebugStringA("WM_CREATE\n");
    }
    break;

    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    }
    break;

    case WM_PAINT:
    {
      OutputDebugStringA("WM_PAINT\n");
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int x = Paint.rcPaint.left;
      int y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.left - Paint.rcPaint.right;
      PatBlt(DeviceContext, x, y, Width, Height, WHITENESS);
      EndPaint(Window, &Paint);
    }
    break;

    case WM_DESTROY:
    {

      OutputDebugStringA("WM_DESTROY\n");
    }
    break;

    default:
    {
      return DefWindowProc(Window, Message, WParam, LParam);
    }
  }

  return (Result);
}

typedef struct _WNDCLASS
{
  UINT style;
  WNDPROC lpfnWndProc;
  int cbClsExtra;
  int cbWndExtra;
  HANDLE Instance;
  HICON Icon;
  HCURSOR Cursor;
  HBRUSH hbrBackground;
  LPCTSTR lpszMenuName;
  LPCTSTR lpszClassName;
} WNDCLSS;

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR LpCmdLine, int NShowCmd)
{
  WNDCLASS WindowClass = {};
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";
  if (RegisterClass(&WindowClass))
  {
    OutputDebugStringA("Works as intended!");
    HWND WindowHandle = CreateWindowExA(
        0, WindowClass.lpszClassName, "Handmade Hero",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
    if (WindowHandle)
    {
      for (;;)
      {

        MSG Message;
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if (MessageResult > 0)
        {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      // TODO: LOG
    }
  }
  else
  {
    // TODO: LOG
  }

  return (0);
}
