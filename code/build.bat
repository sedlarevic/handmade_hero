@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

set code_path= ..\handmade\code\

:: GENERAL COMPILER FLAGS
set compiler=            -Oi     &:: Use assembly intrinsics where possible
set compiler= %compiler% -MT     &:: Include C Runtime Library (CRT Library) in the executable (static link (functions are physically copied inside the executable))
set compiler= %compiler% -nologo &:: Supress Startup Banner
set compiler= %compiler% -Gm-    &:: Disable minimal rebuild
set compiler= %compiler% -GR-    &:: Disable runtime info (C++)
set compiler= %compiler% -EHa-   &:: Disable exception handling (C++)
set compiler= %compiler% -W4     &:: Display warnings up to level 4
set compiler= %compiler% -WX     &:: Treat all warnings as errors

:: IGNORE WARNINGS
set compiler= %compiler% -wd4201 &:: Nameless struct/union
set compiler= %compiler% -wd4100 &:: Unused function parameter
set compiler= %compiler% -wd4189 &:: Local variable not referenced
set compiler= %compiler% -wd4505 &:: Unreferenced local function has been removed (dead code)  NOTE: Maybe remove? 

:: DEBUG VARIABLES
set debug=         -FC           &:: Produce the full path of the source code file
set debug= %debug% -Z7           &:: Produce debug information

:: WIN32 LINKER SWITCHES
set win32_link=              -subsystem:windows,5.2 &:: subsystem,5.1 for x86
set win32_link= %win32_link% -opt:ref               &:: Remove unused functions

:: WIN32 PLATFORM LIBRARIES
set win32_libs=              user32.lib
set win32_libs= %win32_libs% gdi32.lib

:: CROSS_PLATFORM DEFINES
set defines=           -DHANDMADE_INTERNAL=1
set defines= %defines% -DHANDMADE_SLOW=1

:: No optimizations (slow): -Od; all optimizations (fast): -O2
cl -Od -DHANDMADE_WIN32=1 %compiler% %debug% -Fmwin32_handmade.map  %defines% %code_path%win32_handmade.cpp %win32_libs% /link %win32_link%

popd
