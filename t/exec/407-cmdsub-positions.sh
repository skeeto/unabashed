# Command substitution in every word position
$(echo echo) cmd-word
"$(echo echo)" quoted-cmd-word
v=$(echo assigned); echo $v
case $(echo b) in a) echo A;; $(echo b)) echo B-matched;; esac
for i in $(echo 1 2) $(echo 3); do printf '%s,' $i; done; echo
if $(echo true); then echo if-true; fi
while $(echo false); do :; done; echo while-done
echo hi > /tmp/$(echo file)
while read l; do echo "read: $l"; done < /tmp/$(echo file)
f() { echo "arg1=$1"; }
f $(echo x y)
[[ $(echo abc) == a* ]] && echo dbl-bracket
echo $(( $(echo 3) + 4 ))
echo ${v:$(echo 2):$(echo 3)}
