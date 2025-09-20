@echo off
rm main.exe
gcc main.c -I"D:/deps/include/" -L"D:/deps/libs/" -lSDL3 -o main.exe