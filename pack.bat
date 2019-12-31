REM To use this you will need to create a "lib" directory in the main application directory.
REM 
REM The following dll's need to be in there for the full application to pack:
REM 	avcodec-58.dll
REM 	avdevice-58.dll
REM 	avfilter-7.dll
REM 	avformat-58.dll
REM 	avutil-56.dll
REM 	postproc-55.dll
REM 	SDL2.dll
REM 	SDL2_net.dll
REM 	swresample-3.dll
REM 	swscale-5.dll
REM 

@echo off

SET folder=watchtogether

mkdir %folder%
mkdir %folder%\bin
xcopy bin\watchtogether.exe %folder%\bin
xcopy lib\*.dll %folder%\bin
xcopy .\connect.bat %folder%\
xcopy .\host.bat %folder%\

7z a watchtogether.zip %folder%\* -y

rd %folder% /s /q
