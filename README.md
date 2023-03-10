# WindowsReparaturTool

Das "Windows Reparatur Tool" (WRT) ist für eine erleichterte Bedienung gedacht, um sein Windows wieder auf Vordermann zu bringen. Das Windows Reparatur Tool nutzt Windows Programme die dafür gedacht sind, Windows auf Fehler zu überprüfen und diese zu beheben. Es werden folgende Befehle dafür genutzt:
- sfc /scannow
- Dism /Online /Cleanup-Image /ScanHealth
- Dism /Online /Cleanup-Image /RestoreHealth
- chkdsk C: /f /r /x /b

Zu beachten ist, dass die System-Partition den Laufwerksbuchstaben "C:" haben muss. Sonst ist das Programm nicht kompatibel.
