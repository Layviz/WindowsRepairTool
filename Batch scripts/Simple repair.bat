defrag C: /o /h
sfc /scannow
"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe" -Scan -ScanType 1
chkdsk C: /scan /perf /i
dism /online /cleanup-image /CheckHealth
Dism /Online /Cleanup-Image /ScanHealth
Dism /Online /Cleanup-Image /RestoreHealth
chkdsk C: /scan /perf
sfc /scannow
defrag C: /o /h