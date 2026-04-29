@echo off
:: ============================================================================
:: Cardputer ADV — Library Installer
::
:: Installs all Arduino libraries required to build the firmware.
:: Requires arduino-cli to be installed and on PATH.
::   https://arduino.github.io/arduino-cli/latest/installation/
:: ============================================================================

echo.
echo Cardputer ADV — Installing Arduino libraries
echo =============================================
echo.

:: Check arduino-cli is available
where arduino-cli >nul 2>&1
if errorlevel 1 (
    echo ERROR: arduino-cli not found on PATH.
    echo        Install it from: https://arduino.github.io/arduino-cli/latest/installation/
    pause
    exit /b 1
)

:: --- Add M5Stack board index (needed for M5Cardputer board package) ----------
echo [1/2] Adding M5Stack board index...
arduino-cli config add board_manager.additional_urls ^
    https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
echo.

:: --- Update library and board indexes ----------------------------------------
echo [2/2] Updating indexes...
arduino-cli core update-index
arduino-cli lib update-index
echo.

:: --- Install M5Stack core (includes M5Cardputer) ------------------------------
echo Installing M5Stack ESP32 core...
arduino-cli core install m5stack:esp32
echo.

:: --- Install libraries from the Arduino Library Manager ----------------------
echo Installing libraries from Arduino Library Manager...
echo.

echo   ArduinoJson (v7)
arduino-cli lib install "ArduinoJson"

echo   TinyGPSPlus
arduino-cli lib install "TinyGPSPlus"

echo   RadioLib
arduino-cli lib install "RadioLib"

echo   IRremote
arduino-cli lib install "IRremote"

echo   DHTesp (Grove: DHT11)
arduino-cli lib install "DHTesp"

echo   OneWire (Grove: DS18B20)
arduino-cli lib install "OneWire"

echo   DallasTemperature (Grove: DS18B20)
arduino-cli lib install "DallasTemperature"

:: --- Install libraries that require a git URL --------------------------------
echo.
echo Installing ESPAsyncWebServer + AsyncTCP from GitHub...
echo (These are not version-pinnable — latest HEAD is fetched.)
echo.

arduino-cli lib install --git-url https://github.com/me-no-dev/ESPAsyncWebServer.git --allow-unsafe
arduino-cli lib install --git-url https://github.com/me-no-dev/AsyncTCP.git --allow-unsafe

:: --- Done --------------------------------------------------------------------
echo.
echo ============================================================
echo All libraries installed.
echo.
echo Build the firmware with:
echo   arduino-cli compile --profile default firmware\cardputer_webapi
echo.
echo Or with explicit flags:
echo   arduino-cli compile ^
echo     --fqbn "m5stack:esp32:m5stack_cardputer:PartitionScheme=no_ota" ^
echo     firmware\cardputer_webapi
echo ============================================================
echo.
pause
