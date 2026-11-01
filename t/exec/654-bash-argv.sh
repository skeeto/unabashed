# BASH_ARGV and BASH_ARGC: set up when first read by name (not by a
# listing) or when extdebug is turned on, from the positional
# parameters then; calls push under extdebug, sourced files push their
# arguments under extdebug, or else their name
declare -p | grep '^declare -a BASH_ARG'
set -- p q
: "${BASH_ARGC}"
declare -p | grep '^declare -a BASH_ARG'
set -- r
declare -p BASH_ARGV
echo 'declare -p BASH_ARGV BASH_ARGC' > s.sh
f() { declare -p BASH_ARGV; . ./s.sh; . ./s.sh x; }
f 1 2
shopt -s extdebug
f 1 2
g() { shift; set -- z; declare -p BASH_ARGV; f 3; }
g a b
( f 4 ); x=$(f 5); echo "$x"
f 6 | cat
echo "${BASH_ARGV[*]} / ${BASH_ARGC[*]} / $BASH_ARGV $BASH_ARGC"
bash -c 'f() { shopt -s extdebug; declare -p BASH_ARGV; }; f 7 8; declare -p BASH_ARGV' zero a
