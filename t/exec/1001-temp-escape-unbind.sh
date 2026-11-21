# A temporary binding before a command goes away however the command
# ends: an error in the function, the builtin, a later assignment or the
# program's expansions unbinds it as finishing would
cat > s.sh <<'EOF'
f() { echo ${y!!}; }
X=1 f
declare -p X
g() { declare -i n; n=1+; }
Y=2 g
declare -p Y
declare -i m
Z=3 W=$((m=1+)) echo hi
declare -p Z W
Q=4 declare m=1+
declare -p Q
P=5 cat ${y!!}
declare -p P
h() { local v=1; }
R=6 h
declare -p R
EOF
bash s.sh; echo "st $?"
