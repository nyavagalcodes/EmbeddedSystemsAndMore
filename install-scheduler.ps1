# Installs Task Scheduler tasks for Embedded Systems Mastery git watcher
# Uses PowerShell Register-ScheduledTask — handles paths with spaces correctly

$vbs  = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\start-watcher.vbs"
$exe  = "wscript.exe"
$args = "`"$vbs`""

$action   = New-ScheduledTaskAction -Execute $exe -Argument $args
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries

# Task 1: Run at logon
$triggerLogon = New-ScheduledTaskTrigger -AtLogOn
Register-ScheduledTask `
    -TaskName "EmbeddedSystems-GitWatcher-Logon" `
    -Action $action `
    -Trigger $triggerLogon `
    -Settings $settings `
    -Force | Out-Null

# Task 2: Keep-alive every 5 minutes
$triggerRepeat = New-ScheduledTaskTrigger -RepetitionInterval (New-TimeSpan -Minutes 5) -Once -At (Get-Date)
Register-ScheduledTask `
    -TaskName "EmbeddedSystems-GitWatcher-KeepAlive" `
    -Action $action `
    -Trigger $triggerRepeat `
    -Settings $settings `
    -Force | Out-Null

Write-Host "Tasks registered. Starting watcher now..." -ForegroundColor Green
Start-Process wscript.exe -ArgumentList "`"$vbs`""
Start-Sleep -Seconds 3
Write-Host "Done! Embedded Systems watcher is running." -ForegroundColor Green
