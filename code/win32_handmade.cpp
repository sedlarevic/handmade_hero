#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

#define internal_function static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359f

#include "handmade.cpp"

#include <dsound.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <xaudio2.h>
#include <xinput.h>

#include "win32_handmade.h"

// GLOBAL VARIABLES

global_variable win32_offscreen_buffer GlobalBackBuffer;

global_variable bool GlobalRunning;
global_variable IDirectSoundBuffer *GlobalSecondaryBuffer;

// ########## UTILS START #############

// ########## UTILS END #############

// ########### FILE IO START #############

internal_function debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename)
{
  debug_read_file_result Result = {};
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0,
                                  OPEN_EXISTING, 0, 0);

  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    if (GetFileSizeEx(FileHandle, &FileSize))
    {
      uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
      Result.Contents =
          VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      if (Result.Contents)
      {
        DWORD BytesRead;
        if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
            (FileSize32 == BytesRead))
        {
          // NOTE: File read successfully.
          Result.ContentsSize = BytesRead;
        }
        else
        {

          // TODO: LOG: File reading failed.
          DEBUGPlatformFreeFileMemory(Result.Contents);
          Result.Contents = 0;
        }
      }
      else
      {
        // TODO: LOG: Memory Allocation failed.
      }
    }
    else
    {
      // TODO: LOG: File size evaluation failed.
    }
    CloseHandle(FileHandle);
  }
  else
  {
    // TODO: LOG: File handle creation failed.
  }

  return (Result);
}

void DEBUGPlatformFreeFileMemory(void *Memory)
{
  if (Memory)
  {
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize,
                                    void *Memory)
{

  bool32 Result = false;
  HANDLE FileHandle =
      CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  if (FileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD BytesWritten;
    if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
    {
      // NOTE: File write successful.
      Result = (BytesWritten == MemorySize);
    }
    else
    {
      // TODO: LOG: File writing failed.
    }
    CloseHandle(FileHandle);
  }
  else
  {
    // TODO: LOG: File Handle creation failed.
  }
  return (Result);
}

// ########### FILE IO END #############

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
  // NOTE: The audio played 2-4 times as fast, I've changed the
  // SamplesPerSecond to 22050, and changed nChannels to 1, and it worked.

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

      BufferDescription.dwSize = sizeof(BufferDescription);
      BufferDescription.dwBufferBytes = BufferSize;
      BufferDescription.lpwfxFormat = &WaveFormat;

      if (SUCCEEDED(DSoundObject->CreateSoundBuffer(&BufferDescription,
                                                    &GlobalSecondaryBuffer, 0)))
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

internal_function void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                            &Region1, &Region1Size, &Region2,
                                            &Region2Size, 0)))
  {
    uint8 *DestSample = (uint8 *)Region1;
    for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
    {

      *DestSample++ = 0;
    }
    DestSample = (uint8 *)Region2;

    for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
    {
      *DestSample++ = 0;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal_function void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock,
                     DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
  VOID *Region1;
  DWORD Region1Size;
  VOID *Region2;
  DWORD Region2Size;
  if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1,
                                            &Region1Size, &Region2,
                                            &Region2Size, 0)))
  {
    int16 *DestSample = (int16 *)Region1;
    int16 *SourceSample = SourceBuffer->Samples;
    DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
    for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
    {

      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }

    DestSample = (int16 *)Region2;

    DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
    for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
    {

      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      ++SoundOutput->RunningSampleIndex;
    }
    GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
  else
  {
  }
}

// ############ DIRECTSOUND STUFF END #############

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
}

internal_function void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext,
                           int WindowWidth, int WindowHeight)
{

  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0,
                Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                DIB_RGB_COLORS, SRCCOPY);
}

internal_function void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                game_button_state *OldState, DWORD ButtonBit,
                                game_button_state *NewState)
{
  NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
  NewState->HalfTransitionCount =
      (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal_function void Win32ProcessKeyboardMessage(game_button_state *NewState,
                                                   bool32 IsDown)
{
  NewState->EndedDown = IsDown;
  ++NewState->HalfTransitionCount;
}

internal_function void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
  MSG Message;

  while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch (Message.message)
    {

      case WM_QUIT:
      {
        GlobalRunning = false;
      }
      break;

      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN:
      case WM_KEYDOWN:
      case WM_KEYUP:
      {
        uint32 VKCode = (uint32)Message.wParam;
        bool WasDown = ((Message.lParam & (1 << 30)) != 0);
        bool IsDown = ((Message.lParam & (1 << 31)) == 0);
        if (WasDown != IsDown)
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
          else if (VKCode == 'Q')
          {
            Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder,
                                        IsDown);
          }
          else if (VKCode == 'E')
          {
            Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder,
                                        IsDown);
          }
          else if (VKCode == VK_UP)
          {
            OutputDebugStringA("UP\n");
            Win32ProcessKeyboardMessage(&KeyboardController->Up, IsDown);
          }
          else if (VKCode == VK_DOWN)
          {
            OutputDebugStringA("DOWN\n");
            Win32ProcessKeyboardMessage(&KeyboardController->Down, IsDown);
          }
          else if (VKCode == VK_LEFT)
          {
            OutputDebugStringA("LEFT\n");
            Win32ProcessKeyboardMessage(&KeyboardController->Left, IsDown);
          }
          else if (VKCode == VK_RIGHT)
          {
            OutputDebugStringA("RIGHT\n");
            Win32ProcessKeyboardMessage(&KeyboardController->Right, IsDown);
          }
          else if (VKCode == VK_ESCAPE)
          {
            OutputDebugStringA("ESCAPE\n");
            GlobalRunning = false;
          }
          else if (VKCode == VK_SPACE)
          {
            OutputDebugStringA("SPACE\n");
          }
          if (WasDown)
          {
            OutputDebugStringA("Was down\n");
          }
          if (IsDown)
          {
            OutputDebugStringA("Is Down\n");
          }
        }

        bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
        if ((VKCode == VK_F4) && AltKeyWasDown)
        {
          OutputDebugStringA("ALT + F4 CLICKED!");
          GlobalRunning = false;
        }
      }
      break;

      default:
      {
        TranslateMessage(&Message);
        DispatchMessageA(&Message);
      }
    }
  }
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM LParam)
{

  LRESULT Result = 0;

  switch (Message)
  {

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_KEYDOWN:
    {
      Assert(!"Keyboard input came in through a non-dispatch message!");
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

      // NOTE: Video Test Vars
      int XOffset = 0;
      int YOffset = 0;
      // NOTE: Audio Test Vars
      win32_sound_output SoundOutput = {};
      SoundOutput.ToneHz = 256;
      SoundOutput.ToneVolume = 1000;
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = sizeof(int16) * 2;
      SoundOutput.RunningSampleIndex = 0;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
      SoundOutput.SecondaryBufferSize =
          2 * SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

      HDC DeviceContext = GetDC(Window);
      Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
      Win32InitDSound(Window, SoundOutput.SamplesPerSecond,
                      SoundOutput.SecondaryBufferSize);
      Win32ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
      GlobalRunning = true;
      // Win32InitXAudio2(Window, 22050);

      int16 *Samples =
          (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

      // Initializing game memory
      game_memory GameMemory = {};

      // NOTE: When in development build, it is more handy to address the
      // memory somewhere in the middle of an address space (0-~4TB). It
      // makes debugging easier, since we have a pointer point at the same
      // location in a section of a program every time.
      // When not in development build, we let VirtualAlloc (OS) handle the
      // addressing of memory.
#if HANDMADE_INTERNAL
      LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID BaseAddress = 0;
#endif
      // Specifying sizes for each memory block
      GameMemory.PermanentStorageSize = Megabytes(64);
      GameMemory.TransientStorageSize = Gigabytes(1);

      uint64 TotalStorageSize =
          GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
      // Allocation of memory (only once), we then offset the transient
      // memory pointer to point after the size of permanent memory.

      /*
       * ____________________________ 0
       * |                          |
       * |                          |
       * |__________________________| 2TB
       * | Permanent storage (64MB) |
       * |__________________________| 2TB 64 MB
       * | Transient storage (2GB)  |
       * |__________________________|
       * |                          |
       * |                          |
       * |__________________________| 4TB
       */

      GameMemory.PermanentStorage =
          VirtualAlloc(BaseAddress, (size_t)TotalStorageSize,
                       MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
      GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage +
                                     GameMemory.PermanentStorageSize);

      if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
      {

        game_input Input[2] = {};
        game_input *NewInput = &Input[0];
        game_input *OldInput = &Input[1];

        // Metrics
        LARGE_INTEGER LastCounter;
        LARGE_INTEGER CycleFrequency;

        QueryPerformanceCounter(&LastCounter);
        QueryPerformanceFrequency(&CycleFrequency);
        uint64 LastCycleCount = __rdtsc();
        while (GlobalRunning)
        {
          game_controller_input *KeyboardController = &NewInput->Controllers[0];
          game_controller_input ZeroController = {};
          *KeyboardController = ZeroController;

          Win32ProcessPendingMessages(KeyboardController);

          DWORD dwResult;
          // Counting active controllers

          DWORD MaxControllerCount = XUSER_MAX_COUNT;
          if (MaxControllerCount > ArrayCount(NewInput->Controllers))
          {
            MaxControllerCount = ArrayCount(NewInput->Controllers);
          }

          for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount;
               ControllerIndex++)
          {
            XINPUT_STATE ControllerState;
            game_controller_input *OldController =
                &OldInput->Controllers[ControllerIndex];
            game_controller_input *NewController =
                &NewInput->Controllers[ControllerIndex];

            // Simply get the state of the controller from XInput.
            dwResult = XInputGetState(ControllerIndex, &ControllerState);

            if (dwResult == ERROR_SUCCESS)
            {
              // Controller is connected
              XINPUT_GAMEPAD *Gamepad = &ControllerState.Gamepad;

              // TODO: DPAD
              bool32 DPAD_UP = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
              bool32 DPAD_DOWN = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
              bool32 DPAD_LEFT = (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
              bool32 DPAD_RIGHT =
                  (Gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

              // Normalizing thumbstick values

              // TODO: Min/Max Macros and Deadzone processing
              NewController->IsAnalog = true;
              NewController->StartX = OldController->EndX;
              NewController->StartY = OldController->EndY;

              real32 X;
              if (Gamepad->sThumbLX < 0)
              {
                X = (real32)Gamepad->sThumbLX / -32768.0f;
              }
              else
              {
                X = (real32)Gamepad->sThumbLX / 32767.0f;
              }
              NewController->MinX = OldController->MaxX = NewController->EndX =
                  X;

              real32 Y;
              if (Gamepad->sThumbLY < 0)
              {
                Y = (real32)Gamepad->sThumbLY / 32768.0f;
              }
              else
              {
                Y = (real32)Gamepad->sThumbLY / 32767.0f;
              }
              NewController->MinY = OldController->MaxY = NewController->EndY =
                  Y;

              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A,
                  &NewController->Down);
              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B,
                  &NewController->Right);
              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X,
                  &NewController->Left);
              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y,
                  &NewController->Up);
              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->LeftShoulder,
                  XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
              Win32ProcessXInputDigitalButton(
                  Gamepad->wButtons, &OldController->RightShoulder,
                  XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

              // TODO: Add changing sound by pressing a gamepad button.
            }
            else
            {
              // Controller is not connected
            }
          }
          // Defining Bytes to write to

          //          Play C  Target C ByteToLock
          // case 1: |-----I-I-------------I--------|
          // When Play Cursor is behind then we can overwrite the buffer in
          // 2 regions, since this is a circular buffer:
          //  Play C Target C  ByteToLock  Secondary Buffer Size
          // |-----I-I-------------I@@@@@@@@| <- first region to overwrite
          // |@@@@@I@I-------------I--------| <- second region to overwrite
          //
          //          ByteToLock      Play C  Target C
          // case 2: |-----I---------------I-I------|
          // When ByteToLock is behind a Play Cursor, then we can overwrite
          // the buffer to the byte of the Target Cursor, which is a little
          // bit ahead of a Play Cursor, to mitigate latency.
          //    ByteToLock    Play C Target C
          // |-----I@@@@@@@@@@@@@@@I@I------| <------ first and only region
          // to overwrite If successful, we get the play cursor and write
          // cursor position.

          DWORD ByteToLock;
          DWORD BytesToWrite;
          DWORD TargetCursor;
          DWORD PlayCursor;
          DWORD WriteCursor;
          bool32 SoundIsValid = false;

          if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(
                  &PlayCursor, &WriteCursor)))
          {
            // NOTE: We are using a mod because we are working with a
            // circular buffer.
            ByteToLock =
                (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                SoundOutput.SecondaryBufferSize;
            TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount *
                                           SoundOutput.BytesPerSample)) %
                            SoundOutput.SecondaryBufferSize);
            // Calculate how many bytes are there to write to.
            if (ByteToLock > TargetCursor)
            {
              // Play cursor is behind
              BytesToWrite =
                  SoundOutput.SecondaryBufferSize - ByteToLock; // region 1
              BytesToWrite += TargetCursor;                     // region 2
            }
            else
            {
              BytesToWrite = TargetCursor - ByteToLock; // region 1
            }

            SoundIsValid = true;

            game_sound_output_buffer SoundBuffer = {};
            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
            SoundBuffer.Samples = Samples;

            game_offscreen_buffer GameOffscreenBuffer = {};
            GameOffscreenBuffer.Memory = GlobalBackBuffer.Memory;
            GameOffscreenBuffer.Height = GlobalBackBuffer.Height;
            GameOffscreenBuffer.Width = GlobalBackBuffer.Width;
            GameOffscreenBuffer.Pitch = GlobalBackBuffer.Pitch;
            GameUpdateAndRender(&GameMemory, NewInput, &GameOffscreenBuffer,
                                &SoundBuffer);

            if (SoundIsValid)
            {
              Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite,
                                   &SoundBuffer);
            }
          }
          win32_window_dimension Dimension = Win32GetWindowDimension(Window);
          Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                     Dimension.Width, Dimension.Height);
          ++XOffset;
          ++YOffset;

          // Metrics evaluation
          //
          LARGE_INTEGER EndCounter;
          QueryPerformanceCounter(&EndCounter);
          int64 EndCycleCount = __rdtsc();

          int64 CountsElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
          int64 CyclesElapsed = EndCycleCount - LastCycleCount;
          // Frames per Second
          real32 FPS = (real32)CycleFrequency.QuadPart / (real32)CountsElapsed;
          // MegaCycles per Frame
          real32 MCPF = ((real32)CyclesElapsed / (1000.0f * 1000.0f));
          // Milliseconds per Frame
          real32 MSPerFrame = 1000.0f * ((real32)CountsElapsed /
                                         (real32)CycleFrequency.QuadPart);

          // Example:
          /*
           * FPS (f/s): 123.87 -> If 1 Frame took 8 milliseconds, we can
           * divide 1000 (milliseconds in a second) / 8.07, and we get FPS.
           * MSPerFrame (ms/f): 8.07 -> In a given frame we took 8
           * milliseconds. MCPF (mc/f): 25.77 -> Executed 25 Million
           * instructions per frame.
           */

#if 0
        char Buffer[256];
        snprintf(Buffer, 255,
                 "FPS (f/s): %.02f\nMSPerFrame (ms/f): %.02f\nMCPF (mc/f): "
                 "%.02f\n\n",
                 FPS, MSPerFrame, MCPF);
        OutputDebugStringA(Buffer);
#endif
          LastCounter = EndCounter;
          LastCycleCount = EndCycleCount;

          game_input *Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;
        }
      }
      else
      {
        // TODO: LOG: Memory allocation failed.
      }
    }
    else
    {
      // TODO: LOG: Window creation failed.
    }
  }
  else
  {
    // TODO: LOG: Registration of window class failed.
  }

  return (0);
}
