@echo off
echo Restarting Embedded Systems watcher...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*EmbeddedSystemsMastery*watch-and-push*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"
timeout /t 2 /nobreak >nul
wscript.exe "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\start-watcher.vbs"
echo Watcher restarted.
pause
