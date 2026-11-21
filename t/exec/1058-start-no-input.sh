# A shell to read its commands from a closed fd 0 fails at once, as
# bash finds no input to buffer; with -c or a script it runs as usual
bash <&-
echo "status $?"
bash -s a b <&-
echo "status $?"
echo 'echo "script $#"' >s.sh
bash s.sh 1 2 <&-
echo "status $?"
bash -c 'echo "command $0"' named <&-
echo "status $?"
