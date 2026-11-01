# nocasematch and =~
shopt -s nocasematch
[[ ABC =~ abc ]]; echo 1 $? "${BASH_REMATCH[0]}"
[[ abc =~ ^[A-C]+$ ]]; echo 2 $?
[[ abc =~ [[:upper:]]+ ]]; echo 3 $? "${BASH_REMATCH[0]}"
[[ aBc =~ [[:lower:]]+ ]]; echo 4 $? "${BASH_REMATCH[0]}"
[[ xAy =~ x(a)y ]]; echo 5 $? "${BASH_REMATCH[1]}"
[[ aA =~ (a)\1 ]]; echo 6 $?
[[ a =~ [^A] ]]; echo 7 $?
re='[Z-a]'; [[ a =~ $re ]]; echo 8 $?
re='\a'; [[ A =~ $re ]]; echo 9 $?
re='\w+'; [[ ' Ab_9 ' =~ $re ]]; echo 10 $? "<${BASH_REMATCH[0]}>"
re='[[.a.]]'; [[ A =~ $re ]]; echo 11 $?
shopt -u nocasematch
[[ ABC =~ abc ]]; echo 12 $?
re='[Z-a]'; [[ '^' =~ $re ]]; echo 13 $?
echo done
