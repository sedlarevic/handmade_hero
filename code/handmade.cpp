#include <cmath>
#include <cstdint>

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

internal_function void GameOutputSound(game_sound_output_buffer *SoundBuffer,
                                       int ToneHz)
{
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
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

void GameUpdateAndRender(game_memory *Memory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;

  if (!Memory->IsInitialized)
  {
    GameState->ToneHz = 256;
    GameState->XOffset = 0;
    GameState->YOffset = 0;
    Memory->IsInitialized = true;
  }
  game_controller_input *Input0 = &Input->Controllers[0];
  if (Input0->IsAnalog)
  {
    // TODO: Use analog movement tuning
    GameState->ToneHz = 256 + (int)(128.0f * (Input0->EndY));
    GameState->YOffset += (int)4.0f * (Input0->EndX);
  }
  else
  {
    // TODO: Use digital movement tuning
  }

  if (Input0->Down.EndedDown)
  {
    GameState->XOffset += 1;
  }
  // TODO: Allow sample offsets here for more robust platform options.
  GameOutputSound(SoundBuffer, GameState->ToneHz);
  RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);
}
