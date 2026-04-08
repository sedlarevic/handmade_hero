#include <stdint.h>
#include <windows.h>

#define internal_function static
#define local_persist static
#define global_variable static

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 int32_t
#define uint64 uint64_t
#define int8 int8_t
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal_function void RenderWeirdGradient1(int XOffset, int YOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  uint8 *Row = (uint8 *)BitmapMemory;
  int Pitch = Width * BytesPerPixel;
  for (int Y = 0; Y < Height; ++Y)
  {

    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Width; ++X)
    {
      uint8 Red = X + XOffset;
      uint8 Blue = Y + YOffset;
      *Pixel++ = (Blue << 16) | Red;
    }
    Row += Pitch;
  }
}

internal_function void RenderWeirdGradient2(int XOffset, int YOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  uint8 *Row = (uint8 *)BitmapMemory;
  int Pitch = BytesPerPixel * Width;
  for (int Y = 0; Y < Height; Y++)
  {

    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Width; X++)
    {
      uint8 Green = 0;
      uint8 Blue = Y + YOffset;
      uint8 Red = X + XOffset;
      *Pixel++ = Blue | (Green << 8) | (Red << 16);
    }
    Row += Pitch;
  }
}

internal_function void Win32ResizeDIBSection(int Width, int Height)
{
  if (BitmapMemory)
  {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (Width * Height) * BytesPerPixel;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
  RenderWeirdGradient1(Width, Height);
}

internal_function void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect)
{
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;

  StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0,
                WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
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
      PAINTSTRUCT Paint;
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      HDC DeviceContext = BeginPaint(Window, &Paint);
      Win32UpdateWindow(DeviceContext, &ClientRect);
      EndPaint(Window, &Paint);
      OutputDebugStringA("WM_PAINT\n");
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
    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, Instance, 0);
    if (Window)
    {
      Running = true;
      int XOffset = 0;
      int YOffset = 0;
      while (Running)
      {

        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
          if (Message.message == WM_QUIT)
          {
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        RECT ClientRect;
        HDC DeviceContext = GetDC(Window);
        GetClientRect(Window, &ClientRect);
        RenderWeirdGradient2(XOffset, YOffset);
        Win32UpdateWindow(DeviceContext, &ClientRect);
        ReleaseDC(Window, DeviceContext);
        ++XOffset;
        ++YOffset;
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
