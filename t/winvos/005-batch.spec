== stdout 230
["a b"] [%PATH%] ["x&y"] ["a b" "%PATH%" "x&y"]
status 4
[""] [plain] ["q""q"] ["" plain "q""q"]
status 127
status 127
status 126
C:\Windows\System32\cmd.exe C:\Users\oracle\AppData\Local\Temp ^x %undefined%
status 7
status 49

== stderr 233
bash: line 10: t.bat: command not found
bash: line 12: ./nosuch.bat: No such file or directory
bash: line 15: ./dir.cmd: Is a directory
'nosuch' is not recognized as an internal or external command,
operable program or batch file.

== merged 463
["a b"] [%PATH%] ["x&y"] ["a b" "%PATH%" "x&y"]
status 4
[""] [plain] ["q""q"] ["" plain "q""q"]
bash: line 10: t.bat: command not found
status 127
bash: line 12: ./nosuch.bat: No such file or directory
status 127
bash: line 15: ./dir.cmd: Is a directory
status 126
C:\Windows\System32\cmd.exe C:\Users\oracle\AppData\Local\Temp ^x %undefined%
status 7
'nosuch' is not recognized as an internal or external command,
operable program or batch file.
status 49

== status 0
