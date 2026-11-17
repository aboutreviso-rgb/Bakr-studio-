@echo off
REM Installation automatique du plugin (demande les droits administrateur)
net session >nul 2>&1 || (powershell -Command "Start-Process -FilePath %~f0 -Verb RunAs" & exit /b)
xcopy /E /I /Y "%~dp0BAKR GUITAR.vst3" "C:\Program Files\Common Files\VST3\BAKR GUITAR.vst3"
if errorlevel 1 (echo ERREUR : copie impossible. & pause & exit /b 1)
echo.
echo BAKR GUITAR est installe dans C:\Program Files\Common Files\VST3
echo Dans FL Studio : Options ^> Manage plugins ^> Find installed plugins.
echo Puis ajoute-le comme instrument (Channel rack ^> + ^> BAKR GUITAR).
pause
