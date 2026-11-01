# read -t: timeouts on pipes that stay empty, partial input, -t 0 to
# test for input, TMOUT as the default, and bad specifications
a=old
sleep 2 | read -t 0.25 a
echo "1 $? <$a>"
echo abc | { read -t 1 a; echo "2 $? <$a>"; }
{ printf 'ab'; sleep 2; echo c; } | { read -t 1 a; echo "3 $? <$a>"; }
{ sleep 2; echo late; } | { read -t 0 a; echo "4 $?"; read -t 5 a; echo "5 $? <$a>"; }
read -t 0 <<<x; echo "6 $?"
read -t 0 </dev/null; echo "7 $?"
echo x | { sleep 1; read -t 0; echo "8 $?"; }
read -t .001 a <<<here; echo "9 $? <$a>"
TMOUT=0.5
sleep 2 | { read a; echo "10 $? <$a>"; }
echo t | { read a; echo "11 $? <$a>"; }
unset TMOUT
read -t -1 a; echo "12 $?"
read -t 1x a; echo "13 $?"
read -t '' a; echo "14 $?"
{ sleep 2; echo z; } | { read -t 0.5 -a arr; echo "15 $? ${#arr[@]}"; }
{ sleep 2; echo z; } | { read -t 0.5 -n 3 a; echo "16 $? <$a>"; }
