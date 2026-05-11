/*This single shared header file will include the operations that the platform
 * layer should be able to perform on behalf of the cross-platform code. We will
 * basically create our own platform instruction API in this header file, and
 * all of our platform non-specific code will call into it this way.*/
#if !defined(HANDMADE_H)
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
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset,
                           int YOffset, game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H
#endif
