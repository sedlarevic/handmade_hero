#include <cmath>
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

#define Pi32 3.14159265359f

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

internal_function void GameOutputSound(game_sound_output_buffer *SoundBuffer)
{
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
  int16 ToneHz = 256;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount;
       SampleIndex++)
  {
    /*
     * NOTE: Since using real32 (float) and not resetting the phase by
     * subtracting 2 pi means the phase will eventually climb up to where the
     * sin() function's phase unwrapping algorithm doesn't have enough bits of
     * quantization, or valid mantissa, left over afterward unwrapping.
     * */

    if (tSine > 2.0f * Pi32)
    {
      tSine -= 2.0 * Pi32;
    }

    real32 SineValue = sinf(tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);

    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    tSine += (2.0f * Pi32 * 1.0f) / (real32)WavePeriod;
  }
}

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset,
                         int YOffset, game_sound_output_buffer *SoundBuffer)
{
  // TODO: Allow sample offsets here for more robust platform options.
  GameOutputSound(SoundBuffer);
  RenderWeirdGradient(Buffer, XOffset, YOffset);
}
