# Variables reach an MSYS2 child unconverted, Windows forms included
# (not in HOME, TMP, PATH...: a child of a non-MSYS2 parent converts
# those itself; issues.md, Accepted)
export FOO='C:\x' BAR=/c/x BAZ='/a:/b'
/usr/bin/printenv FOO BAR BAZ HOME TEMP TMP
BAR='C:\Windows' /usr/bin/printenv BAR
/usr/bin/printenv PATH
