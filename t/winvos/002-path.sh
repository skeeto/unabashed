# personality: native
# Hand-checked baseline of today's minimal Windows layer, not recorded
# from an oracle (M5a step 4); step 7's spawner changes it on purpose.
# PATH is ;-separated and its search tries "" .exe .com .bat .cmd; a
# native program gets an MSVCRT-quoted command line built from argv.
echo "$PATH"
type argv sh cmd busybox
command -v ls
type -P cat.exe
argv a 'b c' '' 'd"e' 'f\' '*'
echo "status $?"
argv32 x
C:/w64/bin/argv.exe y
/c/w64/bin/argv z
nosuch
echo "status $?"
PATH='C:\Windows\System32'
type argv
cmd /c echo %Path%
