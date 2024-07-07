# Windows Repair Tool

The "Windows Repair Tool" (WRT) is designed to make it easier for you to get your Windows up and running again. The Windows Repair Tool uses Windows programs that are designed to check Windows for errors and fix them. The following commands are used for this:
- defrag C: /O /H
- defrag /c /o /h /m
- sfc /scannow
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 1
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 2
- dism /online /cleanup-image /CheckHealth
- dism /online /cleanup-image /ScanHealth
- dism /online /cleanup-image /RestoreHealth
- chkdsk C: /scan /perf
- chkdsk C: /scan /perf /i
- chkdsk C: /scan /perf /f /r /x /b
- chkdsk C: /f /x /spotfix /sdcleanup

Please note that the system partition must have the drive letter "C:". Otherwise the program is not compatible.

## Download Link for the tool:

[Newest WRT version](https://github.com/Layviz/WindowsReparaturTool/releases/download/v1.4.0/WRT_Installer-Version1.4.0.msi)


# Windows Reparatur Tool

Das "Windows Reparatur Tool" (WRT) ist für eine erleichterte Bedienung gedacht, um sein Windows wieder auf Vordermann zu bringen. Das Windows Reparatur Tool nutzt Windows Programme die dafür gedacht sind, Windows auf Fehler zu überprüfen und diese zu beheben. Es werden folgende Befehle dafür genutzt:
- defrag C: /O /H
- defrag /c /o /h /m
- sfc /scannow
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 1
- "%ProgramFiles%\Windows Defender\mpcmdrun.exe" -Scan -ScanType 2
- dism /online /cleanup-image /CheckHealth
- dism /online /cleanup-image /ScanHealth
- dism /online /cleanup-image /RestoreHealth
- chkdsk C: /scan /perf
- chkdsk C: /scan /perf /i
- chkdsk C: /scan /perf /f /r /x /b
- chkdsk C: /f /x /spotfix /sdcleanup

Zu beachten ist, dass die System-Partition den Laufwerksbuchstaben "C:" haben muss. Sonst ist das Programm nicht kompatibel.

## Download Link zum Tool:

[Neuste WRT Version](https://github.com/Layviz/WindowsReparaturTool/releases/download/v1.4.0/WRT_Installer-Version1.4.0.msi)
