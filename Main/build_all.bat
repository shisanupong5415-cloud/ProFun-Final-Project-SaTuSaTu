@echo off
setlocal EnableExtensions DisableDelayedExpansion

REM --- IMPORTANT: Save this file as ANSI or UTF-8 (NO BOM) ---

REM root = this script's directory
set "ROOT=%~dp0"
pushd "%ROOT%"

REM pick compiler
where /q gcc
if errorlevel 1 (
  where /q clang
  if errorlevel 1 (
    echo ERROR: No gcc or clang in PATH.
    goto :end
  ) else (
    set "CC=clang"
  )
) else (
  set "CC=gcc"
)

set "CFLAGS=-std=c11 -Wall -Wextra -O2"
set "SRC=main.c tests_unit.c tests_e2e.c"
set "OUT=app.exe"

echo.
echo Compiling with %CC% ...
"%CC%" %CFLAGS% %SRC% -o "%OUT%"
if errorlevel 1 goto :end

echo.
echo Built "%OUT%"
echo Done.
:end
popd
endlocal
