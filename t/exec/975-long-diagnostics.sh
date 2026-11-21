# Warnings from a script at a long path, with long values and names:
# an unknown locale, a bad BASH_XTRACEFD and a circular nameref
d=$(printf 'd%.0s' {1..200}); d=$d/$d/$d/$d/$d
mkdir -p "$d"
x=$(printf 'x%.0s' {1..300})
y=$(printf 'y%.0s' {1..250})  # glibc takes no locale name over 255
n=$(printf 'n%.0s' {1..300})
cat >"$d/s.sh" <<EOF
LC_ALL=de_DE; echo ok
unset LC_ALL; LC_TIME=$y; echo ok2
BASH_XTRACEFD=$x; echo ok3
declare -n $n=b; declare -n b=$n; echo "[\$$n]"; echo ok4
EOF
bash "$d/s.sh" 2>&1 | sed -e "s|$d|D|" -e "s|$x|X|" -e "s|$y|Y|" -e "s|$n|N|g"
echo status=$?
