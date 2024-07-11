# WindowsReparaturTool

Das "Windows Reparatur Tool" (WRT) ist für eine erleichterte Bedienung gedacht, um sein Windows wieder auf Vordermann zu bringen. Das Windows Reparatur Tool nutzt Windows Programme die dafür gedacht sind, Windows auf Fehler zu überprüfen und diese zu beheben. Es werden folgende Befehle dafür genutzt:
- defrag C: /O /H
- defrag /c /o /h /m
- sfc /scannow
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 1
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 2
- dism /online /cleanup-image /CheckHealth
- dism /online /cleanup-image /ScanHealth
- dism /online /cleanup-image /RestoreHealth
- chkdsk C: /scan /perf /i
- chkdsk C: /scan /perf /f /r /x /b
- chkdsk C: /f /x /spotfix /sdcleanup

Zu beachten ist, dass die System-Partition den Laufwerksbuchstaben "C:" haben muss, Win 10 Oder 11 als x64 sein muss. Sonst ist das Programm nicht kompatibel.

## Download Link zum Tool:

[Neuste WRT Version](https://github.com/Layviz/WindowsReparaturTool/releases/download/v1.4.1/WRT_Installer-Version1.4.1.msi)
