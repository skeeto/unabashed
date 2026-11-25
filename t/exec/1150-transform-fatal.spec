== stdout 1738
-- echo ${x@x}
bash: line 1: ${x@x}: bad substitution
status 127
-- echo ${x@}
bash: line 1: ${x@}: bad substitution
status 127
-- echo ${x@Qx}
bash: line 1: ${x@Qx}: bad substitution
status 127
-- echo "${x@Z}"
bash: line 1: ${x@Z}: bad substitution
status 127
-- a=(1 2); echo ${a[@]@x}
bash: line 1: ${a[@]@x}: bad substitution
status 127
-- a=(); echo ${a[@]@x}

after
status 0
-- echo ${u@x} ${u[@]@x}

after
status 0
-- declare -A m=([k]=v); echo ${m[k]@x}
bash: line 1: ${m[k]@x}: bad substitution
status 127
-- set -- a; echo ${@@x}
bash: line 1: ${@@x}: bad substitution
status 127
-- echo ${x&}
bash: line 1: ${x&}: bad substitution
status 1
-- f() { echo ${x@x}; echo in; }; f
bash: line 1: ${x@x}: bad substitution
status 127
-- eval 'echo ${x@x}; echo in'
bash: line 1: ${x@x}: bad substitution
status 127
-- echo 'echo ${x@x}; echo in' >src.sh; . ./src.sh
./src.sh: line 1: ${x@x}: bad substitution
status 127
-- [[ ${x@x} ]]
bash: line 1: ${x@x}: bad substitution
status 127
-- ( echo ${x@x}; echo in ); echo "sub $?"
bash: line 1: ${x@x}: bad substitution
sub 1
after
status 0
-- y=$(echo ${x@x}; echo in); echo "comsub $? <$y>"
bash: line 1: ${x@x}: bad substitution
comsub 1 <>
after
status 0
-- echo ${x@x} | cat; echo "pipe $?"
bash: line 1: ${x@x}: bad substitution
pipe 0
after
status 0
-- set -e; echo ${x@x}
bash: line 1: ${x@x}: bad substitution
status 1
-- set -o posix; echo ${x&}
bash: line 1: ${x&}: bad substitution
status 127
-- trap 'echo ${x@x}; echo intrap' EXIT; exit 4
bash: line 1: ${x@x}: bad substitution
status 4
-- trap 'echo ${u?}; echo intrap' EXIT; exit 5
bash: line 1: u: parameter not set
status 5
-- trap 'echo ${x&}; echo intrap' EXIT; exit 6
bash: line 1: ${x&}: bad substitution
status 6

== stderr 0

== merged 1738
-- echo ${x@x}
bash: line 1: ${x@x}: bad substitution
status 127
-- echo ${x@}
bash: line 1: ${x@}: bad substitution
status 127
-- echo ${x@Qx}
bash: line 1: ${x@Qx}: bad substitution
status 127
-- echo "${x@Z}"
bash: line 1: ${x@Z}: bad substitution
status 127
-- a=(1 2); echo ${a[@]@x}
bash: line 1: ${a[@]@x}: bad substitution
status 127
-- a=(); echo ${a[@]@x}

after
status 0
-- echo ${u@x} ${u[@]@x}

after
status 0
-- declare -A m=([k]=v); echo ${m[k]@x}
bash: line 1: ${m[k]@x}: bad substitution
status 127
-- set -- a; echo ${@@x}
bash: line 1: ${@@x}: bad substitution
status 127
-- echo ${x&}
bash: line 1: ${x&}: bad substitution
status 1
-- f() { echo ${x@x}; echo in; }; f
bash: line 1: ${x@x}: bad substitution
status 127
-- eval 'echo ${x@x}; echo in'
bash: line 1: ${x@x}: bad substitution
status 127
-- echo 'echo ${x@x}; echo in' >src.sh; . ./src.sh
./src.sh: line 1: ${x@x}: bad substitution
status 127
-- [[ ${x@x} ]]
bash: line 1: ${x@x}: bad substitution
status 127
-- ( echo ${x@x}; echo in ); echo "sub $?"
bash: line 1: ${x@x}: bad substitution
sub 1
after
status 0
-- y=$(echo ${x@x}; echo in); echo "comsub $? <$y>"
bash: line 1: ${x@x}: bad substitution
comsub 1 <>
after
status 0
-- echo ${x@x} | cat; echo "pipe $?"
bash: line 1: ${x@x}: bad substitution
pipe 0
after
status 0
-- set -e; echo ${x@x}
bash: line 1: ${x@x}: bad substitution
status 1
-- set -o posix; echo ${x&}
bash: line 1: ${x&}: bad substitution
status 127
-- trap 'echo ${x@x}; echo intrap' EXIT; exit 4
bash: line 1: ${x@x}: bad substitution
status 4
-- trap 'echo ${u?}; echo intrap' EXIT; exit 5
bash: line 1: u: parameter not set
status 5
-- trap 'echo ${x&}; echo intrap' EXIT; exit 6
bash: line 1: ${x&}: bad substitution
status 6

== status 0
