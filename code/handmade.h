/*This single shared header file will include the operations that the platform
 * layer should be able to perform on behalf of the cross-platform code. We will
 * basically create our own platform instruction API in this header file, and
 * all of our platform non-specific code will call into it this way.*/

/*
 * NOTE:
 * HANDMADE_INTERNAL:
 *  0 - build for public release
 *  1 - build for developer only
 * HANDMADE_SLOW:
 *  0 - no slow code allowed!
 *  1 - slow code allowed (assertions and such)
 */

#if !defined(HANDMADE_H)

// NOTE: If expression is not true (it fails), just write to the null pointer.
// Writing to null pointer crashes the program.
#if HANDMADE_SLOW
#define Assert(Expression)                                                     \
  if (!(Expression))                                                           \
  {                                                                            \
    *(int *)0 = 0;                                                             \
  }
#else
Assert(Expression)
#endif

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
  uint32 ContentsSize;
  void *Contents;
};

internal_function debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename);
internal_function void DEBUGPlatformFreeFileMemory(void *Memory);
internal_function bool32 DEBUGPlatformWriteEntireFile(char *Filename,
                                                      uint32 MemorySize,
                                                      void *Memory);
#endif

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateUInt64(uint64 Value)
{
  Assert(Value <= 0xFFFFFFFF);
  uint32 Result = (uint32)Value;
  return (Result);
}

struct game_offscreen_buffer
{
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct game_sound_output_buffer
{
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

struct game_button_state
{
  int HalfTransitionCount;
  bool32 EndedDown;
};

struct game_controller_input
{
  bool32 IsAnalog;

  real32 StartX;
  real32 StartY;

  real32 MinX;
  real32 MinY;
  real32 MaxX;
  real32 MaxY;

  real32 EndX;
  real32 EndY;

  union
  {
    game_button_state Buttons[6];

    struct
    {
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
};

struct game_input
{
  game_controller_input Controllers[4];
};

struct game_memory
{

  uint64
      PermanentStorageSize; // NOTE: Required to be cleared to zero at startup.
  void *PermanentStorage;
  uint64 TransientStorageSize;
  void *TransientStorage;
  bool32 IsInitialized;
};

void GameUpdateAndRender(game_memory *Memory, game_input *Input,
                         game_offscreen_buffer *Buffer,
                         game_sound_output_buffer *SoundBuffer);

//
//
//

struct game_state
{
  int ToneHz;
  int XOffset;
  int YOffset;
};

#define HANDMADE_H
#endif
