# bash expands arithmetic text ($((...)), ((...)), for ((...)), $[...],
# a subscript, a substring offset or length) as a word only when it has
# a $, a backquote or a ~ in it: then a double-quoted part is a word of
# its own whose \" is read as a quote (Q_ARITH), and a subscript is
# expanded once, its result backslash-quoted and its double quotes kept.
# Other text only loses its quotes, as string_quote_removal does: \"
# stays as ", single quotes and \ before other characters stay.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
done <<'EOF'
y=3; echo $(( "\"$y" ))
echo $(( "\"3" ))
y=3; echo $(( \"$y ))
y=3; echo "$(( "\"$y" ))"
echo $(( "\"3"+~0 ))
echo $(( "\"3"+`echo 1` ))
y=3; echo $(( "1\"+\"$y" ))
y=3; echo $(( "a\"b\"c"+$y ))
y=3; echo $(( "\\\"$y" ))
y=3; echo $(( "\a$y" ))
echo $(( "\a3" ))
y=3; echo $(( "\$y" ))
y=3; (( x = "\"$y" )); echo $x
y=3; for (( i = "\"$y"; i < 5; i++ )); do echo $i; done
y=3; echo $[ "\"$y" ]
y=3; let x="\"$y"; echo $x
y=3; a=(5 6 7 8); echo ${a["\"$y"-2]}
a=(5 6 7 8); echo ${a["\"3"-2]}
y=3; a=(5 6 7 8); a["\"$y"-2]=9; echo ${a[*]}
y=3; a=(5 6 7 8); echo $(( a["\"$y"-2] ))
a=(5 6 7 8); echo $(( a["\"3"-2] ))
a=(5 6 7 8); k='"1"'; echo $(( a[$k] ))
a=(5 6 7 8); echo $(( a[\"1\"] ))
a=(5 6 7 8); echo $(( a[\"1\"] + $# ))
a=(5 6 7 8); b='a[\"3-2]'; echo $(( b ))
a=(5 6 7 8); b='a["3"-2]'; echo $(( b ))
declare -A A=([k]=5 ['"k']=6); y=k; echo $(( A["\"$y"] + 0 )) $(( A["\"k"] + 0 ))
y=3; s=abcdef; echo ${s:"\"$y"} ${s:1:"\"$y"}
s=abcdef; echo ${s:"\"3"}
s=abcdef; echo ${s:'1'}
s=abcdef; echo ${s:\1}
s=abcdef; echo ${s:"1"}
declare -a b=(1 2); [[ -v "b[\"1]" ]]; echo $?
y=1; declare -a b=(1 2); [[ -v "b[\"\$y]" ]]; echo $?
declare -a b=(1 2); [[ -v b["\"1"] ]]; echo $?
EOF
