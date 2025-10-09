@echo off
setlocal enableextensions enabledelayedexpansion
chcp 65001>nul

REM ===================== Resolve ROOT & SRC DIR =====================
set "ROOT=%~dp0"
REM try Main\ first (ตามโครงที่คุณใช้), ถ้าไม่มีก็ใช้โฟลเดอร์เดียวกับสคริปต์
if exist "%ROOT%Main\main.c" (
  set "SRC_DIR=%ROOT%Main"
) else (
  set "SRC_DIR=%ROOT%"
)

set "SRC1=%SRC_DIR%\main.c"
set "SRC2=%SRC_DIR%\tests_unit.c"
set "SRC3=%SRC_DIR%\tests_e2e.c"
set "OUT=%SRC_DIR%\app.exe"

echo [INFO] Source dir  : "%SRC_DIR%"
echo [INFO] Output exe  : "%OUT%"

REM ===================== Locate GCC =====================
set "GCC=gcc"
where gcc >nul 2>nul
if errorlevel 1 (
  if exist "C:\msys64\mingw64\bin\gcc.exe" (
    set "GCC=C:\msys64\mingw64\bin\gcc.exe"
  ) else (
    echo [ERROR] gcc not found. Install MSYS2 MinGW-w64 or add gcc to PATH.
    pause & exit /b 1
  )
)
echo [INFO] Using GCC   : "%GCC%"

REM ===================== Sanity check sources =====================
if not exist "%SRC1%" (
  echo [ERROR] Not found: %SRC1%
  echo        Put build_all.bat at repo root, and ensure Main\main.c exists.
  pause & exit /b 1
)
if not exist "%SRC2%" (
  echo [ERROR] Not found: %SRC2%
  echo        Expect tests_unit.c to be in the same folder as main.c
  pause & exit /b 1
)
if not exist "%SRC3%" (
  echo [ERROR] Not found: %SRC3%
  echo        Expect tests_e2e.c to be in the same folder as main.c
  pause & exit /b 1
)

REM ===================== Compile flags =====================
set "CFG=%~1"
if /I "%CFG%"=="debug" (
  set "CFLAGS=-std=c11 -g -O0 -Wall -Wextra -Wpedantic"
) else (
  set "CFLAGS=-std=c11 -O2 -Wall -Wextra -Wpedantic"
)

REM ===================== Build =====================
if exist "%OUT%" del /f /q "%OUT%" >nul 2>nul
echo [BUILD] "%GCC%" %CFLAGS% -mconsole ^
  "%SRC1%" "%SRC2%" "%SRC3%" -o "%OUT%"

"%GCC%" %CFLAGS% -mconsole ^
  "%SRC1%" "%SRC2%" "%SRC3%" -o "%OUT%"
if errorlevel 1 (
  echo.
  echo [ERROR] Build failed.
  echo        Check that main.c includes prototypes for test fns, e.g.:
  echo          ^#include "tests.h"
  echo        and tests_unit.c / tests_e2e.c implement:
  echo          void run_unit_test_list(void);
  echo          void run_unit_test_search(void);
  echo          void run_e2e_tests(void);
  pause & exit /b 1
)

echo.
echo [OK] Built: "%OUT%"
echo Run it and choose:
echo   6) Unit Test  -> เลือก list หรือ search
echo   7) E2E Test
echo.
pause