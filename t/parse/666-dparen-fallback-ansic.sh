f() { ((echo $'a\001\177'; echo "${x:-$'\001'}") ); }
declare -f
