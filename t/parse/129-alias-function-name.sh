shopt -s expand_aliases
alias def='myfunc'
def() { echo body; }
function def { echo kw; }
declare -f
