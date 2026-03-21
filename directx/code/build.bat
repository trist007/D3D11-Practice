@echo off

echo Compilation started at %date% %time%
echo,

set CommonCompilerFlags=-MT -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4244 -wd4996 -wd4456 -FC -Z7

set CommonLinkerFlags=-incremental:no -opt:ref /DEBUG /PDB:win32_directx.pdb user32.lib gdi32.lib winmm.lib shell32.lib kernel32.lib msvcrt.lib /NODEFAULTLIB:LIBCMT

REM echo Updating etags
REM echo,
REM etags *.cpp *.h raylib\*.c

IF NOT EXIST .\..\build mkdir ..\..\build
pushd ..\..\build

REM delete pdb because debugger maintains a lock on pdb so pdb cannot be overwritten
del *.pdb > NUL 2> NUL

cl %CommonCompilerFlags% ..\directx\code\win32_directx.cpp /link %CommonLinkerFlags%
REM/Fe:win32_directx.exe

popd

echo.
if %errorlevel% equ 0 (
                       echo Compilation finished at %date% %time%
                       ) else (
                               echo Compilation failed with errors at %date% %time%
                               )
