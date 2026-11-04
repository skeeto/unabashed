# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a steps 4, 5);
# step 7 moves batch quoting to the core.
# A batch file runs through ComSpec with BatBadBut quoting: cmd sees
# its arguments quoted, % disarmed and special characters inert. TEMP
# was imported with forward slashes, COMSPEC was not (busybox-w32).
printf '@echo off\r\necho [%%1] [%%~2] [%%3] [%%*]\r\nexit /b 4\r\n' >t.bat
./t.bat 'a b' '%PATH%' 'x&y'
echo "status $?"
./t.bat '' plain 'q"q'
t.bat 1 2
echo "status $?"
./nosuch.bat
echo "status $?"
mkdir dir.cmd
./dir.cmd
echo "status $?"
cmd /c echo %ComSpec% "%TEMP%" ^^x %undefined%
cmd /c exit 7
echo "status $?"
cmd /c nosuch
echo "status $?"
