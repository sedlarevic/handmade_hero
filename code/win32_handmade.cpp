#include <dsound.h>
#include <stdint.h>
#include <windows.h>
#include <xaudio2.h>
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
#define bool32 int64_t

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

// GLOBAL VARIABLES

global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable bool GlobalRunning;


// ############ UTILS STUFF START #############

// ############ UTILS STUFF END #############

// ############ XINPUT STUFF START #############

#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}

X_INPUT_SET_STATE(XInputSetStateStub)
{

  return (ERROR_DEVICE_NOT_CONNECTED);
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal_function void Win32LoadXInput()
{
  HMODULE XInputLibrary = LoadLibraryA("Xinput1_4.dll");
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("Xinput1_3.dll");
  }
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("Xinput9_1_0.dll");
  }

  if (XInputLibrary)
  {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    if (!XInputGetState)
    {
      XInputGetState = XInputGetStateStub;
    }

    XInputSetState =
        (x_input_set_state *)GetProcAddress(XInputLibrary, "XINputSetState");
    if (!XInputSetState)
    {
      XInputSetState = XInputSetStateStub;
    }
  }
  else
  {

    XInputGetState = XInputGetStateStub;
    XInputSetState = XInputSetStateStub;
  }
}

// ############ XINPUT STUFF END #############

// ############ XAUDIO2 STUFF START #############

#define XAUDIO2_CREATE(name)                                                   \
  HRESULT name(IXAudio2 **ppXAudio2, UINT32 Flags,                             \
               XAUDIO2_PROCESSOR XAudio2Processor)

typedef XAUDIO2_CREATE(xaudio2_create);

#ifdef _XBOX // Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX // Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif
internal_function HRESULT Win32XAudio2FindChunk(HANDLE hFile, DWORD fourcc,
                                                DWORD &dwChunkSize,
                                                DWORD &dwChunkDataPosition)
{
  HRESULT hr = S_OK;
  if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    return HRESULT_FROM_WIN32(GetLastError());

  DWORD dwChunkType;
  DWORD dwChunkDataSize;
  DWORD dwRIFFDataSize = 0;
  DWORD dwFileType;
  DWORD bytesRead = 0;
  DWORD dwOffset = 0;

  while (hr == S_OK)
  {
    DWORD dwRead;
    if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
      hr = HRESULT_FROM_WIN32(GetLastError());

    if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
      hr = HRESULT_FROM_WIN32(GetLastError());

    switch (dwChunkType)
    {
      case fourccRIFF:
        dwRIFFDataSize = dwChunkDataSize;
        dwChunkDataSize = 4;
        if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
          hr = HRESULT_FROM_WIN32(GetLastError());
        break;

      default:
        if (INVALID_SET_FILE_POINTER ==
            SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
          return HRESULT_FROM_WIN32(GetLastError());
    }

    dwOffset += sizeof(DWORD) * 2;

    if (dwChunkType == fourcc)
    {
      dwChunkSize = dwChunkDataSize;
      dwChunkDataPosition = dwOffset;
      return S_OK;
    }

    dwOffset += dwChunkDataSize;

    if (bytesRead >= dwRIFFDataSize)
      return S_FALSE;
  }

  return S_OK;
}

internal_function HRESULT Win32XAudio2ReadChunkData(HANDLE hFile, void *buffer,
                                                    DWORD buffersize,
                                                    DWORD bufferoffset)
{
  HRESULT hr = S_OK;
  if (INVALID_SET_FILE_POINTER ==
      SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
    return HRESULT_FROM_WIN32(GetLastError());
  DWORD dwRead;
  if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
    hr = HRESULT_FROM_WIN32(GetLastError());
  return hr;
}

internal_function void Win32InitXAudio2(HWND Window, int32 SamplesPerSecond)
{
  // NOTE: The audio played 2-4 times as fast, I've changed the SamplesPerSecond
  // to 22050, and changed nChannels to 1, and it worked.

  // 1. Load a library
  HMODULE XAudio2Library = LoadLibraryA("xaudio2_9.dll");
  if (XAudio2Library)
  {
    OutputDebugStringA("Successfully loaded XAudio2Library!!\n");
    // 2. Get XAudio2Create Object
    xaudio2_create *XAudio2Create =
        (xaudio2_create *)GetProcAddress(XAudio2Library, "XAudio2Create");


    IXAudio2 *XAudio2Object;
    HRESULT Error = XAudio2Create(&XAudio2Object, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (XAudio2Create && !Error)
    {
      // 3. Create a mastering voice
      IXAudio2MasteringVoice *XAudio2MasteringVoice;
      XAudio2Object->CreateMasteringVoice(&XAudio2MasteringVoice);
      // 4. Defining a format
      WAVEFORMATEX WaveFormat{};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 1;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign =
          WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
      WaveFormat.nAvgBytesPerSec =
          WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;
      // 5. Creating a Source voice
      IXAudio2SourceVoice *XAudio2SourceVoice;
      XAudio2Object->CreateSourceVoice(&XAudio2SourceVoice, &WaveFormat);
      // 6. Filling a buffer.
      char *WavFileName = "..\\handmade\\data\\wav\\africa-toto.wav";
      // 6.1. Open the file
      HANDLE File = CreateFile(WavFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, 0, NULL);

      if (INVALID_HANDLE_VALUE != File &&
          INVALID_SET_FILE_POINTER != SetFilePointer(File, 0, NULL, FILE_BEGIN))
      {
        DWORD ChunkSize;
        DWORD ChunkPosition;
        Win32XAudio2FindChunk(File, fourccRIFF, ChunkSize, ChunkPosition);
        DWORD Filetype;
        Win32XAudio2ReadChunkData(File, &Filetype, sizeof(DWORD),
                                  ChunkPosition);
        if (Filetype == fourccWAVE)
        {
          // 6.2. Fill out the audio data buffer with the contents of the
          // fourccDATA chunk
          Win32XAudio2FindChunk(File, fourccDATA, ChunkSize, ChunkPosition);
          BYTE *DataBuffer = new BYTE[ChunkSize];
          Win32XAudio2ReadChunkData(File, DataBuffer, ChunkSize, ChunkPosition);


          XAUDIO2_BUFFER XAudio2Buffer{};
          XAudio2Buffer.Flags = XAUDIO2_END_OF_STREAM;
          XAudio2Buffer.AudioBytes = ChunkSize;
          XAudio2Buffer.pAudioData = DataBuffer;
          XAudio2Buffer.PlayBegin = 0;
          XAudio2Buffer.PlayLength = 0;
          XAudio2Buffer.LoopBegin = 0;
          XAudio2Buffer.LoopLength = 0;
          XAudio2Buffer.LoopCount = 0;

          // 7. Submit the buffer to the source voice, and start the voice.
          XAudio2SourceVoice->SubmitSourceBuffer(&XAudio2Buffer, NULL);
          XAudio2SourceVoice->Start(0);
        }
        else
        {
          // TODO: Diagnostic
          OutputDebugStringA("Filetype != fourccWAVE\n");
        }
      }
      else
      {
        // TODO: Diagnostic
        OutputDebugStringA("Something went wrong with WAV file loading. NOTE: "
                           "Check the absolute path, from BufferLength\n");
      }
    }
    else
    {
      OutputDebugStringA("Error while calling XAudioCreate\n");
    }
  }
}

// ############ XAUDIO2 STUFF END #############


// ############ DIRECTSOUND STUFF START #############

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal_function void Win32InitDSound(HWND Window, int32 SamplesPerSecond,
                                       int32 BufferSize)
{
  // 1. Load DirectSound Library
  HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");
  if (DirectSoundLibrary)
  {
    OutputDebugStringA("Successfully loaded DirectSoundLibrary!\n");
    // 2. Get DirectSound Object
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(DirectSoundLibrary,
                                              "DirectSoundCreate");
    IDirectSound *DSoundObject;

    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DSoundObject, 0)))
    {

      OutputDebugStringA("Successfully called DirectSoundCreate\n");
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.nBlockAlign =
          WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
      WaveFormat.nAvgBytesPerSec =
          WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

      if (SUCCEEDED(DSoundObject->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
      {

        OutputDebugStringA("Succesfully set Cooperative level.\n");
        // 3. "Create" a Primary buffer

        DSBUFFERDESC BufferDescription = {};
        IDirectSoundBuffer *PrimaryBuffer;

        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        if (SUCCEEDED(DSoundObject->CreateSoundBuffer(&BufferDescription,
                                                      &PrimaryBuffer, 0)))
        {

          OutputDebugStringA("Successfully created a Primary buffer.\n");
          // 3.1. Setup of a Primary buffer
          if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
          {

            OutputDebugStringA(
                "Successfully set the format of a Primary buffer.\n");
          }
          else
          { // TODO: Diagnostic
            OutputDebugStringA(
                "Error while setting a format of a Primary buffer.\n");
          }
        }
        else
        {
          // TODO: Diagnostic
          OutputDebugStringA("Error while creating a Primary buffer.\n");
        }
      }
      else
      {
        // TODO: Diagnostic
        OutputDebugStringA("Error while setting Cooperative level.\n");
      }
      // 4. "Create" Secondary buffer
      DSBUFFERDESC BufferDescription = {};
      IDirectSoundBuffer *SecondaryBuffer;

      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;

      if (SUCCEEDED(DSoundObject->CreateSoundBuffer(&BufferDescription,
                                                    &SecondaryBuffer, 0)))
      {

        OutputDebugStringA("Successfully created a Secondary buffer.\n");

        // 5. Play sound
      }
      else
      {
        // TODO: Diagnostic
        OutputDebugStringA("Error while creating a Secondary buffer.\n");
      }
    }
    else
    {
      // TODO: Diagnostic
      OutputDebugStringA("Error while calling DirectSoundCreate.\n");
    }
  }
  else
  {
    // TODO: Diagnostic

    OutputDebugStringA("Error while loading DirectSoundLibrary.\n");
  }
}

// ############ DIRECTSOUND STUFF END #############

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
      bool32 AltKeyWasDown = ((LParam & (1 << 29) != 0));
      if ((VKCode == VK_F4) && AltKeyWasDown)
      {
        OutputDebugStringA("ALT + F4 CLICKED!");
        GlobalRunning = false;
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
  Win32LoadXInput();
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

      int SamplesPerSecond = 48000;
      int BytesPerSample = sizeof(int16) * 2;
      int SecondaryBufferSize = 2 * SamplesPerSecond * BytesPerSample;

      HDC DeviceContext = GetDC(Window);
      Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
      Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);
      Win32InitXAudio2(Window, 22050);
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

        DWORD dwResult;
        for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT;
             ControllerIndex++)
        {
          XINPUT_STATE ControllerState;

          // Simply get the state of the controller from XInput.
          dwResult = XInputGetState(ControllerIndex, &ControllerState);

          if (dwResult == ERROR_SUCCESS)
          {
            // Controller is connected
            XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;
            bool A = (Gamepad->wButtons & XINPUT_GAMEPAD_A);
            bool B = (Gamepad->wButtons & XINPUT_GAMEPAD_B);
            bool X = (Gamepad->wButtons & XINPUT_GAMEPAD_X);
            bool Y = (Gamepad->wButtons & XINPUT_GAMEPAD_Y);
            bool LEFT_SHOULDER =
                (Gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool RIGHT_SHOULDER =
                (Gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool DPAD_UP = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool DPAD_DOWN = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool DPAD_LEFT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool DPAD_RIGHT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool START = (Gamepad->wButtons & XINPUT_GAMEPAD_START);
            bool BACK = (Gamepad->wButtons & XINPUT_GAMEPAD_BACK);
            uint16 StickX = Gamepad->sThumbLX;
            uint16 StickY = Gamepad->sThumbLY;
          }
          else
          {
            // Controller is not connected
          }
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
