# Tilde expansion inside ${...} words, case patterns, here-strings,
# redirections and for lists; its result is not globbed or split.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show ${u-~} ${u-~/x} "${u-~}" ${u-'~'}
show ${u:-~+}
case /home/oracle in ~) echo case-tilde;; *) echo case-no;; esac
read -r r <<< ~/h
show "$r"
for w in ~ ~/f; do printf '[%s]' "$w"; done; echo
echo data > ~/file
read -r r < ~/file
show "$r"
HOME='/tmp/a b*'
: > /tmp/a; : > '/tmp/a bc'
show ~
show ~/
HOME=/home/oracle
x=~
show "$x" $x
y='~'
show $y
