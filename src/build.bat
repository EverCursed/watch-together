mkdir "..\bin"
pushd "D:\\Program Files (x86)\Microsoft Visual Studio 14.0\VC"
call vcvarsall.bat amd64
popd
cl watchtogether.c /Fe"../bin/watchtogether.exe" User32.lib Gdi32.lib /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup

