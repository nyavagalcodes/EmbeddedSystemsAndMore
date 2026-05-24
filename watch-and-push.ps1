$dir     = "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery"
$trigger = "$dir\push.trigger"
$msgFile = "$dir\push.message"
$logFile = "$dir\push.log"

# Ensure gh is on PATH
$env:PATH += ";C:\Program Files\GitHub CLI"

# Outer loop: if the inner loop ever crashes/exits, restart it automatically
while ($true) {
    try {
        while ($true) {
            if (Test-Path $trigger) {
                Remove-Item $trigger -Force -ErrorAction SilentlyContinue

                # Read optional commit message
                $msg = "Update: Embedded Systems Mastery docs"
                if (Test-Path $msgFile) {
                    $msg = (Get-Content $msgFile -Raw).Trim()
                    Remove-Item $msgFile -Force -ErrorAction SilentlyContinue
                }

                $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
                Add-Content $logFile "[$timestamp] Trigger detected. Committing: $msg"

                Set-Location $dir

                # Clear lock files
                Remove-Item "$dir\.git\HEAD.lock"   -Force -ErrorAction SilentlyContinue
                Remove-Item "$dir\.git\index.lock"  -Force -ErrorAction SilentlyContinue
                Remove-Item "$dir\.git\config.lock" -Force -ErrorAction SilentlyContinue

                # Stage, commit, push
                git -C $dir add -A 2>&1 | Out-Null
                $commitOut = git -C $dir -c user.name="Navaneet" -c user.email="yavagal.navaneet@gmail.com" commit -m $msg 2>&1
                $pushOut   = git -C $dir push origin main 2>&1

                $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
                if ($LASTEXITCODE -eq 0) {
                    Add-Content $logFile "[$timestamp] SUCCESS - pushed to GitHub"
                } else {
                    Add-Content $logFile "[$timestamp] FAILED - $pushOut"
                }
            }
            Start-Sleep -Seconds 5
        }
    } catch {
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Add-Content $logFile "[$timestamp] WATCHER CRASHED ($($_.Exception.Message)) - restarting in 10s"
        Start-Sleep -Seconds 10
        # outer while ($true) restarts the inner loop
    }
}
