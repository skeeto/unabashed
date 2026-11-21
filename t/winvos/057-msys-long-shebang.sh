# personality: msys
# Hand-checked baseline, not recorded from an oracle.
# A #! script at a long path with a long interpreter line: the line
# outlives the sniff that read it while the interpreter and its
# argument go into the command line.
long=$(printf '%0150d' 0)
arg=$(printf 'a%.0s' {1..60})
d=/c/work/dir$long
mkdir $d
cp /c/w64/bin/argv.exe $d/argv.exe
printf '#!%s %s\n' "$d/argv.exe" "$arg" >$d/script$long
chmod +x $d/script$long
$d/script$long 1 | sed "s/$long/L/g; s/$arg/A/g"
$d/script$long 2 | sed "s/$long/L/g; s/$arg/A/g"
