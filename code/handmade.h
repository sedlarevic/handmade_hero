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

void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset,
                         int YOffset);

#define HANDMADE_H
#endif
