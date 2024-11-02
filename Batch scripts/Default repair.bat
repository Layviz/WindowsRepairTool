defrag C: /m /h
sfc /scannow
"%ProgramFiles%\\Windows Defender\\mpcmdrun.exe" -Scan -ScanType 2
chkdsk C: /scan /perf
dism /online /cleanup-image /CheckHealth
Dism /Online /Cleanup-Image /ScanHealth
Dism /Online /Cleanup-Image /RestoreHealth
sfc /scannow
defrag /c /o /h /m
cleanmgr /sagerun:152
chkdsk C: /scan /perf /f /r /x /b