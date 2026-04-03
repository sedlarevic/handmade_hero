#include <windows.h>

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd)
{
	MessageBoxEx(NULL, "This is handmade hero!", "Handmade", MB_OK|MB_ICONINFORMATION, 0);
	return(0);
}
