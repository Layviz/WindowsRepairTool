# WindowsReparaturTool

Das "Windows Reparatur Tool" (WRT) ist für eine erleichterte Bedienung gedacht, um sein Windows wieder auf Vordermann zu bringen. Das Windows Reparatur Tool nutzt Windows Programme die dafür gedacht sind, Windows auf Fehler zu überprüfen und diese zu beheben. Es werden folgende Befehle dafür genutzt:
- defrag C: /O /H
- sfc /scannow
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 1
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 2
- dism /online /cleanup-image /CheckHealth
- dism /online /cleanup-image /ScanHealth
- dism /online /cleanup-image /RestoreHealth
- chkdsk C: /f /r /x /b

Zu beachten ist, dass die System-Partition den Laufwerksbuchstaben "C:" haben muss. Sonst ist das Programm nicht kompatibel.

Download Link zum Tool:
https://github.com/Layviz/WindowsReparaturTool/releases/download/v1.2.3/WRT_Installer-Version1.2.3.msi
