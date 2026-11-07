# Path modes on the real file system. As WORKDIR/bin/unabashed.exe,
# with no etc/fstab, the shell is the msys persona: WORKDIR is / and /c
# is C:. Copied to sh.exe it is the native persona.
top=$PWD
topw=$(pwd -W)
echo "PWD $top"
case $topw in [A-Z]:/*/013-paths) echo "pwd -W native";; *) echo "pwd -W $topw";; esac

# cd and pwd, logical and physical, and back through OLDPWD
mkdir -p a/b
cd a/b && echo "${PWD#"$top"}"
cd ../.. && echo "${PWD#"$top"}."
cd "$topw/a" && echo "${PWD#"$top"}"
cd - >/dev/null && echo "${PWD#"$top"}." "${OLDPWD#"$top"}"
p=$(cd a && pwd -P) && echo "${p#"$top"}"

# \\?\ paths to the same directory, in both spellings
long='\\?\'${topw//\//\\}
[ -d "$long\\a\\b" ] && echo "long dir"
echo hello >"$long\\a\\f"
read -r l <a/f && echo "long write: $l"
read -r l <"//?/$topw/a/f" && echo "long read: $l"
cd "$long\\a" && echo "${PWD#"$top"}"
cd "$top"

# The null device, by every name, both ways
echo gone 2>nul >/dev/null 1>&2
echo "nul $?"
nosuchcmd 2>nul
echo "status $?"
read -r l </dev/null
echo "read null $?"
n=0; while read -r l; do n=$((n+1)); done <NUL; echo "lines $n"
[ -e a/nul ] && echo "a/nul exists"

# A trailing dot is part of a literal msys name
echo dot >'t.'
echo t*
[ -e t. ] && echo "t. exists"
[ -e t ] || echo "t does not"
read -r l <'t.' && echo "read $l"
rm -f "$long\\t."
[ -e t. ] || echo "t. removed"

# The native persona: Windows paths kept, /x on the current drive
# (PATH reaches it in msys form until step 7 converts the environment)
cp "$U" sh.exe
./sh.exe -c 'cd a/b && case $PWD in [A-Z]:/*/013-paths/a/b) echo "native PWD";; esac
mkdir MixCase && cd mixcase && echo "${PWD##*/}"
cd / && echo "$PWD"
echo $OSTYPE'
# busybox-w32's import, values slashed, only for a Windows parent's
# environment (a mixed-case Path, ComSpec or ProgramData)
"$BB" env -i 'Path=C:\a\b' 'X=C:\c' ./sh.exe -c 'echo "$PATH $X"'
"$BB" env -i 'PATH=C:\a\b' 'X=C:\c' ./sh.exe -c 'echo "$PATH $X"'
rm -f sh.exe
