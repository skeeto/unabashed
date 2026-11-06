# The history file: history -a -n -r -w, HISTFILESIZE truncation,
# timestamps with HISTTIMEFORMAT, and the save at exit
cd /tmp
printf 'old1\nold2\n\nold3\nunterminated' > h1
bash -c 'set -o history; history -r h1; echo "r $?"; history; history -r nosuch; echo "r $?"; history -r /dev/null; echo "r $?"'
rm -f h2
bash -c 'set -o history; echo a; history -a h2; echo b; history -a h2; printf "x1\nx2\n" >> h2; history -n h2; history; history -w h3'
cat h2; echo --; cat h3; echo --
printf '#100\nstamped\n#200\nmulti\nline\n\n#300\n\nlast\n' > h4
HISTTIMEFORMAT='[%s] ' bash -c 'set -o history; history -c; history -r h4; history | sed "s/1[0-9]\{9\}/NOW/"; history -w h5'
sed 's/^#1[0-9]\{9\}$/#NOW/' h5; echo --
printf '1\n2\n3\n4\n5\n' > h6
bash -c 'HISTFILE=/tmp/h6 HISTFILESIZE=2; cat /tmp/h6; HISTFILESIZE=0; wc -c < /tmp/h6'
bash -c 'HISTFILE=; history -a; echo "a $?"; history -w ""; echo "w $?"; unset HISTFILE; history -a; echo "a $?"'
bash -c 'set -o history; history -a /nonexistent/x; echo "a $?"'
printf 'old\n' > h7
bash -c 'HISTFILE=/tmp/h7; set -o history; echo hi; trap "echo trap" EXIT; HISTFILESIZE=3'
cat h7; echo --
printf 'old1\nold2\n' > h8
bash -c 'HISTFILE=/tmp/h8; set -o history; echo a; HISTSIZE=1; printf "ext\n" >> /tmp/h8; echo b'
cat h8
