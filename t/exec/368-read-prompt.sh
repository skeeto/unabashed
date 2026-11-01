# read -p prints the prompt only when input is a terminal
read -p "prompt> " x <<<"value"
echo "x=$x"
read -p "again: " -r y </tmp/nosuch
echo "status $?"
read -sp "secret" z <<<"hidden"
echo "z=$z"
