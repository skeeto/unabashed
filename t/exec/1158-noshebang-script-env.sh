# A script without #! runs as a new shell would from the environment it
# is given: a variable exported while unset is not there, nor one not
# exported, nor an array; exported functions are, and the shell's own
# variables are back to their defaults.
cat >f2 <<'EOF'
echo "y=[$y] z=[$z] t=[$t] IFS=[$IFS] PS4=[$PS4] OPTIND=$OPTIND"
declare -p x arr 2>&1
declare -F
shopt -p extglob
EOF
chmod +x f2
export x; unset x; export x
y=1 z=2; export z
arr=(1 2); export arr
f() { :; }; g() { :; }; export -f g
shopt -s extglob
IFS=: PS4=x OPTIND=3
t=4 ./f2
echo ---
IFS=$' \t\n'
bash -c 'declare -p x 2>&1; echo "z=[$z]"'
printf '#!/usr/local/bin/bash\ndeclare -p x 2>/dev/null || echo "no x"\n' >f3
chmod +x f3
./f3
( declare -p x )
declare -p x
x=; ./f2 | sed -n 3p
