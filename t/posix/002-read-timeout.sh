# read -t on input that never runs dry and never ends a line times out
# all the same (the vos has no /dev/zero, and its clock stands still
# while data flows)
read -t 0.05 x </dev/zero; echo "zero $? ${#x}"
yes | { read -t 0.05 -d '' x; echo "yes $?"; }
yes | { read -t 0.05 -N 100000000 x; echo "yes -N $?"; }
