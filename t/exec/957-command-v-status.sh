# command -v and -V with several names succeed if any is found, as in
# bash (command.def's any_found), and say "not found" only for -V; type
# fails if any is missing
f() { :; }
alias al='echo alias'
command -v echo nosuch for f al; echo "v some $?"
command -v nosuch1 nosuch2; echo "v none $?"
command -v nosuch echo; echo "v last $?"
command -V nosuch echo f; echo "V some $?"
command -V nosuch1 nosuch2; echo "V none $?"
command -pv nosuch echo; echo "pv some $?"
type echo nosuch >/dev/null; echo "type some $?"
type -t nosuch for; echo "type -t some $?"
