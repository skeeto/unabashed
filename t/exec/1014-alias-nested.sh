# An alias being expanded is not expanded again by a parse nested in
# its expansion (eval, source, a subshell, a background job or pipeline
# stage), but a command substitution's parse starts afresh, as bash's
# child resets its parser
shopt -s expand_aliases
n=0
alias s1=$'n=$((n+1)); ((n<3)) && (eval s1); echo "sub n=$n"\n'
s1
n=0
alias s2=$'n=$((n+1)); ((n<3)) && x=$(eval s2); echo "comsub n=$n x=$x"\n'
s2
n=0
alias s3=$'n=$((n+1)); ((n<3)) && eval s3; echo "eval n=$n"\n'
s3
n=0
echo 's5' > f5
alias s5=$'n=$((n+1)); ((n<3)) && . ./f5; echo "source n=$n"\n'
s5
n=0
alias s6=$'n=$((n+1)); ((n<3)) && { eval s6 & wait; }; echo "bg n=$n"\n'
s6
n=0
alias s7=$'n=$((n+1)); ((n<3)) && eval s7 | cat; echo "pipe n=$n"\n'
s7
n=0
alias s8=$'n=$((n+1)); ((n<3)) && x=`s8`; echo "backquote n=$n x=$x"\n'
s8
alias s9=$'trap "s9" USR1; kill -USR1 $$; echo trap\n'
s9
trap - USR1
# the trailing-blank rule still sees the next word
alias e='echo ' w='wow '
e w e w
