# Variables reach an MSYS2 child unconverted, Windows forms included
export FOO='C:\x' BAR=/c/x BAZ='/a:/b'
/usr/bin/printenv FOO BAR BAZ HOME TEMP TMP
HOME='C:\Windows' /usr/bin/printenv HOME
/usr/bin/printenv PATH
