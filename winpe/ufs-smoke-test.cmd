@echo off
setlocal EnableExtensions

set "LOG=X:\ufs-smoke-test.txt"
set "DEVICES=X:\ufs-devices.txt"
set "DISKS=X:\ufs-disks.txt"
set "DPSCRIPT=X:\ufs-diskpart.txt"

>"%LOG%" echo Exynos9610 UFS unattended WinPE smoke test
>>"%LOG%" echo ===========================================
>>"%LOG%" echo Date/time: %DATE% %TIME%
>>"%LOG%" ver
>>"%LOG%" echo.

>>"%LOG%" echo [slsiufsstor service]
sc query slsiufsstor >>"%LOG%" 2>&1
>>"%LOG%" echo.
>>"%LOG%" echo [Microsoft StorUFS service]
sc query storufs >>"%LOG%" 2>&1
>>"%LOG%" echo.

pnputil /enum-devices /connected >"%DEVICES%" 2>&1
>>"%LOG%" echo [connected devices matching UFS]
findstr /I /C:"SLSI20A2" /C:"Universal Flash Storage" /C:"slsiufsstor" /C:"storufs" "%DEVICES%" >>"%LOG%" 2>&1
>>"%LOG%" echo.

>"%DPSCRIPT%" echo rescan
>>"%DPSCRIPT%" echo list disk
>>"%DPSCRIPT%" echo list volume
diskpart /s "%DPSCRIPT%" >"%DISKS%" 2>&1
>>"%LOG%" echo [DiskPart read-only enumeration]
type "%DISKS%" >>"%LOG%"
>>"%LOG%" echo.

if exist X:\Windows\INF\setupapi.dev.log (
    >>"%LOG%" echo [SetupAPI lines matching this stack]
    findstr /I /C:"SLSI20A2" /C:"slsiufsstor" /C:"storufs" X:\Windows\INF\setupapi.dev.log >>"%LOG%" 2>&1
    >>"%LOG%" echo.
)

findstr /I /C:"SLSI20A2" /C:"Universal Flash Storage" "%DEVICES%" >nul 2>&1
if errorlevel 1 goto DEVICE_FAIL
findstr /R /C:"Disk [0-9][0-9]* " "%DISKS%" >nul 2>&1
if errorlevel 1 goto DISK_FAIL

>>"%LOG%" echo RESULT: CANDIDATE PASS - UFS device and a physical disk were enumerated.
goto SHOW

:DEVICE_FAIL
>>"%LOG%" echo RESULT: FAIL - ACPI UFS device was not visible to Plug and Play.
goto SHOW

:DISK_FAIL
>>"%LOG%" echo RESULT: FAIL - UFS device appeared, but DiskPart found no physical disk.

:SHOW
cls
color 0F
echo.
echo   Unattended Exynos9610 UFS test - no keyboard required
echo.
type "%LOG%"
echo.
echo   This test only enumerates devices and disks; it does not write to UFS.
echo   Photograph this screen. The complete log is X:\ufs-smoke-test.txt.
echo.
echo   Refreshing in 30 seconds...
timeout /t 30 /nobreak >nul
goto SHOW
