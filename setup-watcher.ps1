# Combined watcher setup — runs once, adapts the existing PersonalWallet scheduler.
# Writes the combined watch-and-push.ps1 to PersonalWallet, restarts the watcher.

$pwPath  = "C:\Navaneet\PersonalWallet\Todo\My Apps\watch-and-push.ps1"
$embPath = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery"
$vbs     = "C:\Navaneet\PersonalWallet\Todo\My Apps\start-watcher.vbs"

# --- 1. Write combined script to PersonalWallet (the scheduler already points here) ---
$combined = @'
# Combined watcher: PersonalWallet + Embedded Systems Mastery
# Kept alive by existing PersonalWallet-GitWatcher-* Task Scheduler tasks.

$projects = @(
    @{
        dir     = "C:\Navaneet\PersonalWallet\Todo\My Apps"
        trigger = "C:\Navaneet\PersonalWallet\Todo\My Apps\push.trigger"
        msgFile = "C:\Navaneet\PersonalWallet\Todo\My Apps\push.message"
        logFile = "C:\Navaneet\PersonalWallet\Todo\My Apps\push.log"
        default = "chore: auto-push via Claude"
    },
    @{
        dir     = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery"
        trigger = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\push.trigger"
        msgFile = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\push.message"
        logFile = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\push.log"
        default = "Update: Embedded Systems Mastery docs"
    }
)

$env:PATH += ";C:\Program Files\GitHub CLI"

while ($true) {
    try {
        while ($true) {
            foreach ($proj in $projects) {
                if (Test-Path $proj.trigger) {
                    Remove-Item $proj.trigger -Force -ErrorAction SilentlyContinue

                    $msg = $proj.default
                    if (Test-Path $proj.msgFile) {
                        $msg = (Get-Content $proj.msgFile -Raw).Trim()
                        Remove-Item $proj.msgFile -Force -ErrorAction SilentlyContinue
                    }

                    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
                    Add-Content $proj.logFile "[$ts] Trigger detected. Committing: $msg"

                    Remove-Item "$($proj.dir)\.git\HEAD.lock"   -Force -ErrorAction SilentlyContinue
                    Remove-Item "$($proj.dir)\.git\index.lock"  -Force -ErrorAction SilentlyContinue
                    Remove-Item "$($proj.dir)\.git\config.lock" -Force -ErrorAction SilentlyContinue

                    git -C $proj.dir add -A 2>&1 | Out-Null
                    git -C $proj.dir -c user.name="Navaneet" -c user.email="yavagal.navaneet@gmail.com" commit -m $msg 2>&1 | Out-Null
                    $pushOut = git -C $proj.dir push origin main 2>&1

                    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
                    if ($LASTEXITCODE -eq 0) {
                        Add-Content $proj.logFile "[$ts] SUCCESS - pushed to GitHub"
                    } else {
                        Add-Content $proj.logFile "[$ts] FAILED - $pushOut"
                    }
                }
            }
            Start-Sleep -Seconds 5
        }
    } catch {
        $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        foreach ($proj in $projects) {
            Add-Content $proj.logFile "[$ts] WATCHER CRASHED ($($_.Exception.Message)) - restarting"
        }
        Start-Sleep -Seconds 10
    }
}
'@

Write-Host "Writing combined watcher to PersonalWallet..." -ForegroundColor Cyan
Set-Content -Path $pwPath -Value $combined -Encoding UTF8
Write-Host "  Done." -ForegroundColor Green

# --- 2. Kill any running watch-and-push process ---
Write-Host "Stopping old watcher..." -ForegroundColor Cyan
Get-CimInstance Win32_Process |
    Where-Object { $_.CommandLine -like '*watch-and-push*' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
Start-Sleep -Seconds 3
Write-Host "  Done." -ForegroundColor Green

# --- 3. Start via existing PersonalWallet VBS (keeps same Task Scheduler chain) ---
Write-Host "Starting combined watcher via existing scheduler VBS..." -ForegroundColor Cyan
Start-Process wscript.exe -ArgumentList "`"$vbs`""
Start-Sleep -Seconds 4

# --- 4. Verify ---
$running = Get-CimInstance Win32_Process |
    Where-Object { $_.CommandLine -like '*watch-and-push*' }

if ($running) {
    Write-Host ""
    Write-Host "SUCCESS - Combined watcher is running (PID $($running.ProcessId))" -ForegroundColor Green
    Write-Host "Watching:" -ForegroundColor Green
    Write-Host "  PersonalWallet  -> push.trigger" -ForegroundColor White
    Write-Host "  Embedded Systems -> push.trigger" -ForegroundColor White

    # Consume the stale trigger that was waiting
    $staleTrigger = "$embPath\push.trigger"
    if (Test-Path $staleTrigger) {
        Write-Host ""
        Write-Host "Found pending trigger - watcher will push now..." -ForegroundColor Yellow
    }
} else {
    Write-Host "WARNING: Watcher process not detected. Try running start-watcher.vbs manually." -ForegroundColor Red
}

Write-Host ""
Write-Host "Setup complete. Press any key to close." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
