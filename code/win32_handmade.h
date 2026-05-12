#if !defined(WIN32_HANDMADE_H)

typedef struct win32_offscreen_buffer
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
} win32_offscreen_buffer;

typedef struct win32_window_dimension
{
  int Height;
  int Width;
} win32_window_dimension;

typedef struct win32_sound_output
{
  int ToneHz;
  int ToneVolume;
  int SamplesPerSecond;
  int BytesPerSample;
  uint32 RunningSampleIndex;
  int SecondaryBufferSize;
  real32 tSine;
  int LatencySampleCount;
} win32_sound_output;

#define WIN32_HANDMADE_H
#endif
