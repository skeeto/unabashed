== stdout 2853
-- y=3; echo $(( "\"$y" ))
3
-- echo $(( "\"3" ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")
-- y=3; echo $(( \"$y ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")
-- y=3; echo "$(( "\"$y" ))"
3
-- echo $(( "\"3"+~0 ))
2
-- echo $(( "\"3"+`echo 1` ))
4
-- y=3; echo $(( "1\"+\"$y" ))
4
-- y=3; echo $(( "a\"b\"c"+$y ))
3
-- y=3; echo $(( "\\\"$y" ))
bash: line 1: \3 : arithmetic syntax error: operand expected (error token is "\3 ")
-- y=3; echo $(( "\a$y" ))
bash: line 1: \a3 : arithmetic syntax error: operand expected (error token is "\a3 ")
-- echo $(( "\a3" ))
bash: line 1: \a3 : arithmetic syntax error: operand expected (error token is "\a3 ")
-- y=3; echo $(( "\$y" ))
bash: line 1: $y : arithmetic syntax error: operand expected (error token is "$y ")
-- y=3; (( x = "\"$y" )); echo $x
3
-- y=3; for (( i = "\"$y"; i < 5; i++ )); do echo $i; done
3
4
-- y=3; echo $[ "\"$y" ]
3
-- y=3; let x="\"$y"; echo $x
bash: line 1: let: x="3: arithmetic syntax error: operand expected (error token is ""3")

-- y=3; a=(5 6 7 8); echo ${a["\"$y"-2]}
6
-- a=(5 6 7 8); echo ${a["\"3"-2]}
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- y=3; a=(5 6 7 8); a["\"$y"-2]=9; echo ${a[*]}
5 9 7 8
-- y=3; a=(5 6 7 8); echo $(( a["\"$y"-2] ))
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- a=(5 6 7 8); echo $(( a["\"3"-2] ))
bash: line 1: a["3-2] : bad array subscript (error token is "a["3-2] ")
-- a=(5 6 7 8); k='"1"'; echo $(( a[$k] ))
bash: line 1: "1": arithmetic syntax error: operand expected (error token is ""1"")
-- a=(5 6 7 8); echo $(( a[\"1\"] ))
6
-- a=(5 6 7 8); echo $(( a[\"1\"] + $# ))
bash: line 1: "1": arithmetic syntax error: operand expected (error token is ""1"")
-- a=(5 6 7 8); b='a[\"3-2]'; echo $(( b ))
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- a=(5 6 7 8); b='a["3"-2]'; echo $(( b ))
6
-- declare -A A=([k]=5 ['"k']=6); y=k; echo $(( A["\"$y"] + 0 )) $(( A["\"k"] + 0 ))
bash: line 1: A["k] + 0 : bad array subscript (error token is "A["k] + 0 ")
-- y=3; s=abcdef; echo ${s:"\"$y"} ${s:1:"\"$y"}
def bcd
-- s=abcdef; echo ${s:"\"3"}
bash: line 1: s: "3: arithmetic syntax error: operand expected (error token is ""3")
-- s=abcdef; echo ${s:'1'}
bash: line 1: s: '1': arithmetic syntax error: operand expected (error token is "'1'")
-- s=abcdef; echo ${s:\1}
bash: line 1: s: \1: arithmetic syntax error: operand expected (error token is "\1")
-- s=abcdef; echo ${s:"1"}
bcdef
-- declare -a b=(1 2); [[ -v "b[\"1]" ]]; echo $?
0
-- y=1; declare -a b=(1 2); [[ -v "b[\"\$y]" ]]; echo $?
0
-- declare -a b=(1 2); [[ -v b["\"1"] ]]; echo $?
bash: line 1: "1: arithmetic syntax error: operand expected (error token is ""1")

== stderr 0

== merged 2853
-- y=3; echo $(( "\"$y" ))
3
-- echo $(( "\"3" ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")
-- y=3; echo $(( \"$y ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")
-- y=3; echo "$(( "\"$y" ))"
3
-- echo $(( "\"3"+~0 ))
2
-- echo $(( "\"3"+`echo 1` ))
4
-- y=3; echo $(( "1\"+\"$y" ))
4
-- y=3; echo $(( "a\"b\"c"+$y ))
3
-- y=3; echo $(( "\\\"$y" ))
bash: line 1: \3 : arithmetic syntax error: operand expected (error token is "\3 ")
-- y=3; echo $(( "\a$y" ))
bash: line 1: \a3 : arithmetic syntax error: operand expected (error token is "\a3 ")
-- echo $(( "\a3" ))
bash: line 1: \a3 : arithmetic syntax error: operand expected (error token is "\a3 ")
-- y=3; echo $(( "\$y" ))
bash: line 1: $y : arithmetic syntax error: operand expected (error token is "$y ")
-- y=3; (( x = "\"$y" )); echo $x
3
-- y=3; for (( i = "\"$y"; i < 5; i++ )); do echo $i; done
3
4
-- y=3; echo $[ "\"$y" ]
3
-- y=3; let x="\"$y"; echo $x
bash: line 1: let: x="3: arithmetic syntax error: operand expected (error token is ""3")

-- y=3; a=(5 6 7 8); echo ${a["\"$y"-2]}
6
-- a=(5 6 7 8); echo ${a["\"3"-2]}
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- y=3; a=(5 6 7 8); a["\"$y"-2]=9; echo ${a[*]}
5 9 7 8
-- y=3; a=(5 6 7 8); echo $(( a["\"$y"-2] ))
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- a=(5 6 7 8); echo $(( a["\"3"-2] ))
bash: line 1: a["3-2] : bad array subscript (error token is "a["3-2] ")
-- a=(5 6 7 8); k='"1"'; echo $(( a[$k] ))
bash: line 1: "1": arithmetic syntax error: operand expected (error token is ""1"")
-- a=(5 6 7 8); echo $(( a[\"1\"] ))
6
-- a=(5 6 7 8); echo $(( a[\"1\"] + $# ))
bash: line 1: "1": arithmetic syntax error: operand expected (error token is ""1"")
-- a=(5 6 7 8); b='a[\"3-2]'; echo $(( b ))
bash: line 1: "3-2: arithmetic syntax error: operand expected (error token is ""3-2")
-- a=(5 6 7 8); b='a["3"-2]'; echo $(( b ))
6
-- declare -A A=([k]=5 ['"k']=6); y=k; echo $(( A["\"$y"] + 0 )) $(( A["\"k"] + 0 ))
bash: line 1: A["k] + 0 : bad array subscript (error token is "A["k] + 0 ")
-- y=3; s=abcdef; echo ${s:"\"$y"} ${s:1:"\"$y"}
def bcd
-- s=abcdef; echo ${s:"\"3"}
bash: line 1: s: "3: arithmetic syntax error: operand expected (error token is ""3")
-- s=abcdef; echo ${s:'1'}
bash: line 1: s: '1': arithmetic syntax error: operand expected (error token is "'1'")
-- s=abcdef; echo ${s:\1}
bash: line 1: s: \1: arithmetic syntax error: operand expected (error token is "\1")
-- s=abcdef; echo ${s:"1"}
bcdef
-- declare -a b=(1 2); [[ -v "b[\"1]" ]]; echo $?
0
-- y=1; declare -a b=(1 2); [[ -v "b[\"\$y]" ]]; echo $?
0
-- declare -a b=(1 2); [[ -v b["\"1"] ]]; echo $?
bash: line 1: "1: arithmetic syntax error: operand expected (error token is ""1")

== status 1
