defrag C: /o /h
sfc /scannow
"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe" -Scan -ScanType 1
sfc /scannow
dism /online /cleanup-image /CheckHealth
Dism /Online /Cleanup-Image /ScanHealth
Dism /Online /Cleanup-Image /RestoreHealth
chkdsk C: /scan /perf /i
sfc /scannow
defrag C: /o /h