@echo off
set /p ip="Partner IP address: "

bin\watchtogether.exe -p %ip%
