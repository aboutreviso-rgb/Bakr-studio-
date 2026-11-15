@echo off
REM BAKR Suite - supprime les plugins BAKR de ce paquet
net session >nul 2>&1 || (powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs" & exit /b)
set "DEST=C:\Program Files\Common Files\VST3"
for /d %%P in ("%~dp0VST3\*.vst3") do (
    if exist "%DEST%\%%~nxP" ( echo Suppression de %%~nxP & rmdir /S /Q "%DEST%\%%~nxP" )
)
echo Termine.
pause
