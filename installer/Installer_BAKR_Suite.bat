@echo off
REM BAKR Suite - installe tous les plugins BAKR d'un coup (version sans assistant)
net session >nul 2>&1 || (powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs" & exit /b)
set "DEST=C:\Program Files\Common Files\VST3"
if not exist "%DEST%" mkdir "%DEST%"
echo Installation de BAKR Suite dans %DEST% ...
for /d %%P in ("%~dp0VST3\*.vst3") do (
    echo   - %%~nxP
    if exist "%DEST%\BAKR\%%~nxP" (
        rmdir /S /Q "%DEST%\BAKR\%%~nxP"
        xcopy /E /I /Y /Q "%%P" "%DEST%\BAKR\%%~nxP" >nul
    ) else (
        xcopy /E /I /Y /Q "%%P" "%DEST%\%%~nxP" >nul
    )
)
echo.
echo Termine ! Dans FL Studio : Options ^> Manage plugins ^> Find installed plugins.
pause
