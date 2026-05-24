@echo off
REM Installs two scheduled tasks (no admin needed):
REM   1. Run at logon
REM   2. Repeat every 5 minutes - start-watcher.vbs checks if already running
REM      and is a no-op if the process is still alive.

set "VBS=C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\start-watcher.vbs"
set "TASK_LOGON=EmbeddedSystems-GitWatcher-Logon"
set "TASK_KEEPALIVE=EmbeddedSystems-GitWatcher-KeepAlive"

REM Remove old tasks
schtasks /delete /tn "%TASK_LOGON%"     /f >nul 2>&1
schtasks /delete /tn "%TASK_KEEPALIVE%" /f >nul 2>&1

REM Task 1: run at logon
schtasks /create /tn "%TASK_LOGON%" /tr "wscript.exe \"%VBS%\"" /sc ONLOGON /it /f /np >nul 2>&1

REM Task 2: repeat every 5 minutes starting now (no-op if already running)
schtasks /create /tn "%TASK_KEEPALIVE%" /tr "wscript.exe \"%VBS%\"" /sc MINUTE /mo 5 /it /f /np >nul 2>&1

echo.
if %errorlevel%==0 (
    echo Tasks created:
    echo   %TASK_LOGON%     - starts watcher at every logon
    echo   %TASK_KEEPALIVE% - keeps it alive, checks every 5 minutes
    echo.
    echo Starting watcher now...
    wscript.exe "%VBS%"
    echo Done. Watcher is running in the background.
) else (
    echo [FAILED] Could not create scheduled tasks.
)
pause
