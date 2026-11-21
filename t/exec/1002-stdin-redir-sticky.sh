# Once a compound command redirects standard input, an asynchronous
# command in the shell keeps its standard input afterwards instead of
# /dev/null, as bash's stdin_redir stays set; a group that redirects
# only output does not set it, and a subshell's is its own
echo input > in.txt
exec < in.txt
cat & wait
echo "1 $?"
( : ) < /dev/null
cat & wait
echo "2"
{ :; } > /dev/null
cat & wait
echo "3 $?"
exec < in.txt
{ :; } < /dev/null
cat & wait
echo "4 $?"
exec < in.txt
cat & wait
echo "5"
