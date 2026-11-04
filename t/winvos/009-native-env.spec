== stdout 441
C:/w64/bin;C:/Windows/System32
C:\Windows\System32\cmd.exe|C:\Windows|C:/Users/oracle/AppData/Local/Temp
C:/Users/oracle|C:/Users/oracle
declare -- 
unset unset
C:/Program Files (x86)
win32 x86_64-w64-mingw32 x86_64 x86_64-w64-mingw32
C:/w64/bin/sh.exe C:/w64/bin/sh.exe
0
0
query 1
msyspath            	off
/bin:/c/Windows/System32
/c/Windows
C:/Windows
msyspath            	on
shopt -s msyspath
0
C:/w64/bin;C:/Windows/System32
C:/Windows

== stderr 59
bash: line 33: shopt: nosuchopt: invalid shell option name

== merged 500
C:/w64/bin;C:/Windows/System32
C:\Windows\System32\cmd.exe|C:\Windows|C:/Users/oracle/AppData/Local/Temp
C:/Users/oracle|C:/Users/oracle
declare -- 
unset unset
C:/Program Files (x86)
win32 x86_64-w64-mingw32 x86_64 x86_64-w64-mingw32
C:/w64/bin/sh.exe C:/w64/bin/sh.exe
0
0
query 1
msyspath            	off
/bin:/c/Windows/System32
/c/Windows
C:/Windows
msyspath            	on
shopt -s msyspath
0
C:/w64/bin;C:/Windows/System32
C:/Windows
bash: line 33: shopt: nosuchopt: invalid shell option name

== status 1
