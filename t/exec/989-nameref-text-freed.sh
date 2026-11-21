# A nameref's subscript, or an indirection's, may unset or reassign the
# nameref whose text is being read: the text stays as it was until the
# expansion is done, and is never read again as another variable's.
# ${!x:=word} resolves the indirection again once word is expanded.
a=(zero one)
tail='+1+0+0+0+0+0+0+0+0+0+0'
S='a[${ unset -n r; zz=$big; yy=$big; }'$tail']'
pre=${S%"$tail]"}
fill=$(printf '%*s' "${#pre}" '' | tr ' ' '0')
big=$fill'$(echo INJECTED)+1'
declare -n r=$S
echo "result: [$r]"
big=0123456789012345678901234567890123456789
a=(zero one two); declare -n r='a[${ unset -n r; zz=$big; }1]'; r=new; declare -p a
a=(zero one two); declare -n r='a[${ unset -n r; zz=$big; }1]'; r+=new; declare -p a
a=(zero one two); declare -n r='a[${ unset -n r; zz=$big; }1]'; unset r; declare -p a
a=(zero one two); declare -n r='a[${ unset -n r; zz=$big; }1]'; printf -v r %s pv; declare -p a
a=(zero one two); declare -n r='a[${ unset -n r; zz=$big; }1]'; read r <<< rd; declare -p a
declare -A h=([k]=v); declare -n r='h[${ unset -n r; zz=$big; }k]'; r=new; declare -p h
x='a[${ x=zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz; }1]'; a=(p q); echo "${!x}"
x=y; unset y; echo "${!x:=${ x=zzzzzzzz; }abc}"; declare -p x zzzzzzzz
unset y
x=q; echo "${!x:=${ x=1; }abc}"
echo "x=$x q=${q-unset}"
x=y; echo "${!x:=${ unset x; }abc}"
echo "y=${y-unset}"
x='a[5]'; echo "${!x:=abc}"
echo "${a[5]-unset}"
