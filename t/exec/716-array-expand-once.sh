# assoc_expand_once: builtin arguments take subscripts as they are
declare -A A
rkey=']' lkey='[' b="80's"
shopt -s assoc_expand_once
printf -v A[$rkey] rbracket
printf -v A[$lkey] lbracket
read A[$b] <<< "Depeche Mode"
read 'A[$HOME]' <<< home
declare -p A
unset A[$rkey] A[$lkey]
declare -p A
for k in ']' '*'; do read "A[$k]" <<< X; done
declare -A myarray; declare myarray["foo[bar"]=bleh myarray[x]=y
declare -p myarray
typeset foo[foo]bar]=bax
declare -A a; let "a[\" \"]=11"; declare -p a
x=0; arr=(5 6); echo $(( arr[arr[0]-5] ))
export subscript='$(echo INJECTION! >&2 ; echo 0)'
( printf -v i["$subscript"] %s hi; echo notreached )
( unset i["$subscript"]; echo notreached )
( test -v i["$subscript"]; echo notreached )
declare plain["foo[bar"]=bleh
echo notreached
