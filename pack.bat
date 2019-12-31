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
