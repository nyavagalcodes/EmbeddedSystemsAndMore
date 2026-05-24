@echo off
cd /d "C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery"

echo === Checking git status ===
git status 2>&1

if exist ".git" (
    echo Git repo already initialized.
) else (
    echo Initializing git repo...
    git init -b main
)

echo === Configuring git user ===
git config user.name "Navaneet Yavagal"
git config user.email "yavagal.navaneet@gmail.com"

echo === Setting remote origin ===
git remote remove origin 2>nul
git remote add origin https://github.com/nyavagalcodes/EmbeddedSystemsAndMore.git

echo === Adding all files ===
git add .

echo === Committing ===
git commit -m "Initial commit: Add all 10 handbook volumes and index"

echo === Pushing to GitHub ===
git push -u origin main

echo === Done ===
pause
