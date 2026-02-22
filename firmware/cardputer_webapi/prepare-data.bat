@echo off
:: Copies the PWA files into firmware/cardputer_webapi/data/ ready for LittleFS upload.
:: After running this, use Arduino IDE: Tools > ESP32 Sketch Data Upload
:: (requires the arduino-esp32fs-plugin or LittleFS data uploader)

set SRC=%~dp0..\pwa
set DST=%~dp0data

echo Syncing %SRC% -> %DST%
if not exist "%DST%" mkdir "%DST%"

robocopy "%SRC%" "%DST%" /E /XD .git /XF *.bat /NFL /NDL /NJH /NJS

echo.
echo Done. Now upload the data/ folder to the Cardputer:
echo   Arduino IDE ^> Tools ^> ESP32 Sketch Data Upload
echo.
