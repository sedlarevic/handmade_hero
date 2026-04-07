#include <windows.h>

#define internal_function static
#define local_persist static
#define global_variable static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal_function void Win32ResizeDIBSection(int Width, int Height)
{
  if (BitmapHandle)
  {
    DeleteObject(BitmapHandle);
  }
  if (!BitmapDeviceContext)
  {
    BitmapDeviceContext = CreateCompatibleDC(0);
  }
  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;
  BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo,
                                  DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}
internal_function void Win32UpdateWindow(HDC DeviceContext, int X, int Y,
                                         int Width, int Height)
{

  StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
                BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}
LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM LParam)
{

  LRESULT Result = 0;

  switch (Message)
  {

    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;

      Win32ResizeDIBSection(Width, Height);
      OutputDebugStringA("WM_SIZE\n");
    }
    break;

    case WM_CLOSE:
    {
      OutputDebugStringA("WM_CLOSE\n");
      // TODO: Will be changed later. Handle as a message to the user?
      Running = false;
    }
    break;

    case WM_PAINT:
    {
      OutputDebugStringA("WM_PAINT\n");
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      Win32UpdateWindow(DeviceContext, X, Y, Height, Width);

      EndPaint(Window, &Paint);
    }
    break;

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;

    case WM_DESTROY:
    {
      // TODO: Will be changed later. Handle as an error, recreate window?
      Running = false;
      OutputDebugStringA("WM_DESTROY\n");
    }
    break;

    default:
    {
      Result = DefWindowProc(Window, Message, WParam, LParam);
    }
    break;
  }

  return (Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR LpCmdLine, int NShowCmd)
{
  WNDCLASS WindowClass = {};
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
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
      Running = true;

      while (Running)
      {

        MSG Message;
        BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
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
