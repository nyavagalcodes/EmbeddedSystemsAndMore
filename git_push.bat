@echo off
cd /d "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery"

git add .

for /f "tokens=*" %%i in ('git status --porcelain') do (
    set CHANGES=1
)

if defined CHANGES (
    git commit -m "Update: Embedded Systems Mastery docs"
    git push origin main
    echo Push complete.
) else (
    echo Nothing to push - working tree is clean.
)
