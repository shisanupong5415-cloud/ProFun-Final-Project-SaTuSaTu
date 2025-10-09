@echo off
setlocal

REM ถ้ามี CC กำหนดไว้แล้ว ใช้อันนั้นเลย
if not "%CC%"=="" goto :use_custom_cc

REM ลองหา gcc ก่อน
where gcc >nul 2>nul
if %errorlevel%==0 (
  echo Using gcc...
  gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -o app.exe main.c tests_unit.c tests_e2e.c
  if %errorlevel% neq 0 exit /b 1
  echo Built app.exe with gcc
  goto :eof
)

REM ถ้าไม่มี gcc ลองหา cl (MSVC)
where cl >nul 2>nul
if %errorlevel%==0 (
  echo Using MSVC cl...
  REM ถ้ามี VS Dev Prompt อยู่แล้วจะมี cl ใช้งานได้
  cl /nologo /std:c11 /W4 /O2 /Fe:app.exe main.c tests_unit.c tests_e2e.c
  if %errorlevel% neq 0 exit /b 1
  echo Built app.exe with MSVC
  goto :eof
)

echo No compiler found (gcc or cl). Install one and retry.
exit /b 1

:use_custom_cc
echo Using custom compiler: %CC%
if "%CFLAGS%"=="" set CFLAGS=-std=c11 -O2 -Wall -Wextra -Wpedantic
%CC% %CFLAGS% -o app.exe main.c tests_unit.c tests_e2e.c
if %errorlevel% neq 0 exit /b 1
echo Built app.exe
