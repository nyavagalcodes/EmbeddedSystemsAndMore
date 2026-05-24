' Only start a new Embedded Systems watcher if one is not already running.
' Uses unique path marker "EmbeddedSystemsMastery" to distinguish from PersonalWallet watcher.
Dim bRunning : bRunning = False
Set objWMI  = GetObject("winmgmts:\\.\root\cimv2")
Set colProcs = objWMI.ExecQuery("SELECT * FROM Win32_Process WHERE CommandLine LIKE '%EmbeddedSystemsMastery%watch-and-push%'")
If colProcs.Count > 0 Then bRunning = True

If Not bRunning Then
    Set objShell = WScript.CreateObject("WScript.Shell")
    objShell.Run "powershell.exe -WindowStyle Hidden -ExecutionPolicy Bypass -File ""C:\Navaneet\EmbeddedSystemsMastery\Embedded System Mastery\watch-and-push.ps1""", 0, False
End If
