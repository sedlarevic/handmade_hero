#include <cstdint>

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

#include "handmade.h"

#define internal_function static
#define local_persist static
#define global_variable static

internal_function void RenderWeirdGradient(game_offscreen_buffer *Buffer,
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

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset,
                         int YOffset)
{
  RenderWeirdGradient(Buffer, XOffset, YOffset);
}

