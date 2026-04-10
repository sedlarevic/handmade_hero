#include <stdint.h>
#include <windows.h>
#include <xinput.h>

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

struct win32_offscreen_buffer
{
  // NOTE: Pixels are always 32-bits wide,
  // Memory Order  0x BB GG RR xx
  // Little Endian 0x xx RR GG BB
  BITMAPINFO Info;
  void *Memory;
  HBITMAP Handle;
  HDC DeviceContext;
  int Width;
  int Height;
  int BytesPerPixel;
  int MemorySize;
  int Pitch;
};

struct win32_window_dimension
{
  int Height;
  int Width;
};

global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable bool GlobalRunning;

internal_function void RenderWeirdGradient1(win32_offscreen_buffer *Buffer,
                                            int XOffset, int YOffset)

{

  uint8 *Row = (uint8 *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y)
  {

    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer->Width; ++X)
    {
      uint8 Red = X + XOffset;
      uint8 Green = 0;
      uint8 Blue = Y + YOffset;
      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal_function void RenderWeirdGradient2(win32_offscreen_buffer *Buffer,
                                            int XOffset, int YOffset)
{

  uint8 *Row = (uint8 *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; Y++)
  {

    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer->Width; X++)
    {

      uint8 Red = X + XOffset;
      uint8 Green = 0;
      uint8 Blue = Y + YOffset;
      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal_function win32_window_dimension Win32GetWindowDimension(HWND Window)
{
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Height = ClientRect.bottom - ClientRect.top;
  Result.Width = ClientRect.right - ClientRect.left;
  return (Result);
}

internal_function void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer,
                                             int Width, int Height)
{
  // NOTE: When the biHeight field is negative, this is the clue
  // to Windows to treat this bitmap as top-down, not bottom-up, meaning
  // that the first bytes of the image are the color for the top left
  // pixel in the bitmap, not the bottom left!

  if (Buffer->Memory)
  {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;
  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
  Buffer->MemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, Buffer->MemorySize, MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
  RenderWeirdGradient1(Buffer, Width, Height);
}

internal_function void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,
                           int WindowWidth, int WindowHeight)
{

  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM LParam)
{

  LRESULT Result = 0;

  switch (Message)
  {

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      uint32 VKCode = WParam;
      bool wasDown = ((LParam & (1 << 30)) != 0);
      bool isDown = ((LParam & (1 << 31)) == 0);
      if (wasDown != isDown)
      {
        if (VKCode == 'W')
        {
          OutputDebugStringA("W\n");
        }

        else if (VKCode == 'A')
        {

          OutputDebugStringA("A\n");
        }
        else if (VKCode == 'S')
        {

          OutputDebugStringA("S\n");
        }
        else if (VKCode == 'D')
        {

          OutputDebugStringA("D\n");
        }
        else if (VKCode == VK_UP)
        {
          OutputDebugStringA("UP\n");
        }
        else if (VKCode == VK_DOWN)
        {

          OutputDebugStringA("DOWN\n");
        }
        else if (VKCode == VK_LEFT)
        {

          OutputDebugStringA("LEFT\n");
        }
        else if (VKCode == VK_RIGHT)
        {

          OutputDebugStringA("RIGHT\n");
        }
        else if (VKCode == VK_ESCAPE)
        {
          OutputDebugStringA("ESCAPE\n");
        }
        else if (VKCode == VK_SPACE)
        {
          OutputDebugStringA("SPACE\n");
        }
        if (wasDown)
        {
          OutputDebugStringA("Was down\n");
        }
        if (isDown)
        {
          OutputDebugStringA("Is Down\n");
        }
      }
    }
    break;

    case WM_PAINT:
    {
      PAINTSTRUCT Paint;

      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      HDC DeviceContext = BeginPaint(Window, &Paint);
      Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                 Dimension.Width, Dimension.Height);
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
      GlobalRunning = false;
      OutputDebugStringA("WM_DESTROY\n");
    }
    break;

    case WM_CLOSE:
    {
      OutputDebugStringA("WM_CLOSE\n");
      // TODO: Will be changed later. Handle as a message to the user?
      GlobalRunning = false;
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
  WindowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
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
      GlobalRunning = true;

      int XOffset = 0;
      int YOffset = 0;
      HDC DeviceContext = GetDC(Window);
      Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

      while (GlobalRunning)
      {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
          if (Message.message == WM_QUIT)
          {
            GlobalRunning = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        RenderWeirdGradient2(&GlobalBackBuffer, XOffset, YOffset);

        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                   Dimension.Width, Dimension.Height);
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
