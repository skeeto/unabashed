# An unaliased name stays out of every listing of the alias table:
# completion's alias names, alias, BASH_ALIASES, type, subshells
shopt -s expand_aliases
alias zz=1 zy=2 e=; unalias zz
compgen -a z; echo "s=$?"
compgen -A alias zz; echo "s=$?"
compgen -a; echo "s=$?"
alias; alias -p
echo "${!BASH_ALIASES[@]} n=${#BASH_ALIASES[@]}"
declare -p BASH_ALIASES
type zz; echo $?; command -v zz; echo $?
(compgen -a z; alias; echo "sub ${#BASH_ALIASES[@]}")
echo "$(compgen -a)"
complete -a foo; compgen -a -X 'e' ''
# A redefined alias keeps its place; one unaliased first takes a new one
alias zz=3 a1=1 a2=2 a3=3 a4=4; unalias a2; alias a2=x a1=y zy=5
echo "${!BASH_ALIASES[@]}"
unalias -a; compgen -a; echo "s=$?"
