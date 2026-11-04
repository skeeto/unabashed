# vos
# A name without .exe finds the .exe in file tests and when run
for f in /usr/bin/ls /usr/bin/ls.exe /usr/bin/LS /bin/ls /usr/bin/nosuch; do
    printf '%s:' "$f"
    [ -e "$f" ] && printf ' e'
    [ -f "$f" ] && printf ' f'
    [ -x "$f" ] && printf ' x'
    [[ -e $f ]] && printf ' E'
    echo
done
a=$(command -v argv)
[ -x "$a" ] && [ -x "$a.exe" ] && echo both
"${a%.exe}" x
