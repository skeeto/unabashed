echo start
f() { cat; } <<EOF; echo same line; declare -f f
heredoc runs to the end
echo not a command
