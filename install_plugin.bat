@echo off
setlocal EnableDelayedExpansion

title Dark Mode C Plugin Manager
:START
cls
echo ========================================================
echo       Dark Mode C Plugin Manager for Notepad++
echo ========================================================
echo.

:: 1. Admin Rights Check
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [STATUS] Running with Administrator privileges. (Good)
) else (
    echo [WARNING] Not running as Administrator.
    echo           Installation to "Program Files" will fail.
    echo           Right-click and "Run as Administrator".
)
echo.

:MAIN_MENU
echo Select an action:
echo [1] Install / Update Plugin
echo [2] Uninstall Plugin
echo [3] Exit
echo.
set /p choice="Enter option (1-3): "

if "%choice%"=="1" goto PRE_INSTALL
if "%choice%"=="2" goto PRE_UNINSTALL
if "%choice%"=="3" exit /b 0
goto MAIN_MENU

:: ========================================================
:: LOCATE NOTEPAD++ LOGIC
:: ========================================================
:LOCATE_NPP
echo.
echo Searching for Notepad++...

:: Try typical locations
:: 64-bit NPP on 64-bit Windows
if exist "%ProgramFiles%\Notepad++\plugins" (
    set "TARGET_DIR=%ProgramFiles%\Notepad++\plugins"
    set "EXPECTED_ARCH=x64"
    goto %RETURN_LABEL%
)
:: 32-bit NPP on 64-bit Windows
if exist "%ProgramFiles(x86)%\Notepad++\plugins" (
    set "TARGET_DIR=%ProgramFiles(x86)%\Notepad++\plugins"
    set "EXPECTED_ARCH=x86"
    goto %RETURN_LABEL%
)
:: 32-bit NPP on 32-bit Windows
if exist "C:\Program Files\Notepad++\plugins" (
    set "TARGET_DIR=C:\Program Files\Notepad++\plugins"
    set "EXPECTED_ARCH=x86"
    goto %RETURN_LABEL%
)

echo [WARNING] Could not automatically find Notepad++ plugins folder.
:CUSTOM_PATH
echo.
echo Please enter the full path to your Notepad++ "plugins" folder.
echo Example: C:\MyApps\Notepad++\plugins
echo.
set /p "TARGET_DIR=Path: "
set "TARGET_DIR=%TARGET_DIR:"=%"
set "EXPECTED_ARCH=Unknown"

if not exist "%TARGET_DIR%" (
    echo [ERROR] The path "%TARGET_DIR%" does not exist!
    goto CUSTOM_PATH
)
goto %RETURN_LABEL%

:: ========================================================
:: CHECK PROCESS LOGIC
:: ========================================================
:CHECK_PROCESS
tasklist /FI "IMAGENAME eq notepad++.exe" 2>NUL | find /I /N "notepad++.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo.
    echo [WARNING] Notepad++ is currently running!
    echo           The plugin file cannot be updated while in use.
    echo.
    echo [1] Close Notepad++ automatically
    echo [2] I have closed it manually, continue
    echo [3] Cancel
    echo.
    set /p killchoice="Option: "
    if "!killchoice!"=="1" (
        echo [INFO] Asking Notepad++ to close gracefully...
        taskkill /IM notepad++.exe >nul 2>&1
        timeout /t 3 /nobreak >nul
        tasklist /FI "IMAGENAME eq notepad++.exe" 2>NUL | find /I "notepad++.exe" >nul
        if not errorlevel 1 (
            echo [INFO] Still running. Forcing close ^(unsaved changes may be lost^)...
            taskkill /F /IM notepad++.exe >nul 2>&1
            timeout /t 1 /nobreak >nul
        )
    )
    if "!killchoice!"=="3" goto START
)
exit /b 0

:: ========================================================
:: INSTALLER
:: ========================================================
:PRE_INSTALL
if not exist "dark_mode_C.dll" (
    echo.
    echo [ERROR] dark_mode_C.dll not found in this folder!
    pause
    goto START
)

:: DLL VALIDATION & ARCHITECTURE CHECK
echo Validating DLL...
set "DLL_ARCH=Unknown"
for /f "usebackq" %%A in (`powershell -NoProfile -Command "$b=Get-Content 'dark_mode_C.dll' -Encoding Byte -TotalCount 2048; $off=[BitConverter]::ToInt32($b,60); $m=[BitConverter]::ToUInt16($b,$off+4); if($m -eq 0x8664){'x64'}elseif($m -eq 0x014c){'x86'}else{'Unknown'} "`) do set "DLL_ARCH=%%A"

echo [INFO] DLL Architecture detected: %DLL_ARCH%

set "RETURN_LABEL=DO_INSTALL"
goto LOCATE_NPP

:DO_INSTALL
if "%EXPECTED_ARCH%"=="Unknown" goto SKIP_ARCH_CHECK
if "%DLL_ARCH%"=="Unknown" goto SKIP_ARCH_CHECK

if not "%DLL_ARCH%"=="%EXPECTED_ARCH%" (
    echo.
    echo [CRITICAL WARNING] Architecture Mismatch!
    echo ----------------------------------------
    echo Target Folder seems to be: %EXPECTED_ARCH%
    echo Your DLL file is:          %DLL_ARCH%
    echo.
    echo Notepad++ will likely FAIL to load this plugin.
    echo.
    set /p ignore="Continue anyway? (Y/N): "
    if /I not "!ignore!"=="Y" goto START
)

:SKIP_ARCH_CHECK
call :CHECK_PROCESS
set "PLUGIN_DIR=%TARGET_DIR%\dark_mode_C"

echo.
echo Target: "%PLUGIN_DIR%"

:: Backup existing
if exist "%PLUGIN_DIR%\dark_mode_C.dll" (
    echo [INFO] Creating backup of existing version...
    copy /Y "%PLUGIN_DIR%\dark_mode_C.dll" "%PLUGIN_DIR%\dark_mode_C.dll.bak" >nul
)

:: Create folder
if not exist "%PLUGIN_DIR%" (
    mkdir "%PLUGIN_DIR%"
    if errorlevel 1 (
        echo [ERROR] Access Denied. Please run as Administrator.
        pause
        goto START
    )
)

:: Copy file
copy /Y "dark_mode_C.dll" "%PLUGIN_DIR%\"
if errorlevel 1 (
    echo [ERROR] Failed to copy file. Access Denied or file locked.
    pause
    goto START
)

echo.
echo [SUCCESS] Plugin installed/updated successfully!
echo.
pause
goto START

:: ========================================================
:: UNINSTALLER
:: ========================================================
:PRE_UNINSTALL
set "RETURN_LABEL=DO_UNINSTALL"
goto LOCATE_NPP

:DO_UNINSTALL
call :CHECK_PROCESS
set "PLUGIN_DIR=%TARGET_DIR%\dark_mode_C"

if not exist "%PLUGIN_DIR%" (
    echo.
    echo [INFO] Plugin folder not found. Nothing to uninstall.
    pause
    goto START
)

echo.
echo [WARNING] This will delete: "%PLUGIN_DIR%"
set /p confirm="Are you sure? (Y/N): "
if /I not "%confirm%"=="Y" goto START

rmdir /s /q "%PLUGIN_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to delete folder. Check permissions or if file is in use.
    pause
    goto START
)

echo.
echo [SUCCESS] Plugin uninstalled.
echo.
pause
goto START
