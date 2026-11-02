# invalid alias names; unaliased names are not expanded
exec 2>&1
shopt -s expand_aliases
alias '\$'=xx; echo $?
alias 'a b=x' 'a/b=x' 'a$=x' 'a;=x' 'a"=x' 'a<=x' ok-name=x; echo $?
BASH_ALIASES['\$']=xx; echo $?
BASH_ALIASES[a=b]=x; echo $?
alias
unalias -a
alias foo=bar
alias a1='echo '
a1 foo
unalias foo
a1 foo
type foo
alias e='echo ' f='foo ' b=bar x=xtra
e f b x
