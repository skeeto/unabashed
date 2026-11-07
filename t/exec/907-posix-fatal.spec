== stdout 2129
bash: line 1: set: nosuch: invalid option name
st 2
bash: line 1: unset: -q: invalid option
unset: usage: unset [-f] [-v] [-n] [name ...]
st 2
bash: line 1: export: `1a': not a valid identifier
st 1
bash: line 1: export: `1a': not a valid identifier
after 1
st 0
bash: line 1: unset: `1a': not a valid identifier
bash: line 1: unset: `2b': not a valid identifier
st 1
bash: line 1: unset: r: cannot unset: readonly variable
st 1
bash: line 1: r: readonly variable
st 1
bash: line 1: unset: f: cannot unset: readonly function
st 1
bash: line 1: export: nosuch: not a function
after 1
st 0
bash: line 1: shift: 9: shift count out of range
after 1
st 0
bash: line 1: shift: x: numeric argument required
st 2
bash: line 1: return: can only `return' from a function or sourced script
st 2
bash: line 1: return: x: numeric argument required
st 2
bash: line 1: set: nosuch: invalid option name
or
after 0
st 0
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: set: nosuch: invalid option name
after 0
st 0
bash: line 1: set: nosuch: invalid option name
or
after 0
st 0
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: set: nosuch: invalid option name
after 2 []
st 0
bash: line 1: set: nosuch: invalid option name
st 2
bash: eval: line 2: syntax error: unexpected end of file from `(' command on line 1
st 2
bash: line 1: /nonexist: No such file or directory
st 1
bash: line 1: /nonexist: No such file or directory
after 1
st 0
bash: line 1: r: readonly variable
st 127
bash: line 1: r: readonly variable
st 127
bash: line 1: r: readonly variable
st 1
bash: line 2: r: readonly variable
next 1
bash: line 4: r: readonly variable
next 1
bash: line 7: r: readonly variable
next 1
st 0
bash: line 1: `a-b': not a valid identifier
st 2
bash: line 1: r: readonly variable
st 127
1) 1
#? bash: line 1: r: readonly variable
st 127
bash: line 1: ${x!y}: bad substitution
st 127
bash: line 1: set: nosuch: invalid option name
st 2
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: r: readonly variable
after 0
st 0
bash: line 1: `a-b': not a valid identifier
after 1
st 0

== stderr 0

== merged 2129
bash: line 1: set: nosuch: invalid option name
st 2
bash: line 1: unset: -q: invalid option
unset: usage: unset [-f] [-v] [-n] [name ...]
st 2
bash: line 1: export: `1a': not a valid identifier
st 1
bash: line 1: export: `1a': not a valid identifier
after 1
st 0
bash: line 1: unset: `1a': not a valid identifier
bash: line 1: unset: `2b': not a valid identifier
st 1
bash: line 1: unset: r: cannot unset: readonly variable
st 1
bash: line 1: r: readonly variable
st 1
bash: line 1: unset: f: cannot unset: readonly function
st 1
bash: line 1: export: nosuch: not a function
after 1
st 0
bash: line 1: shift: 9: shift count out of range
after 1
st 0
bash: line 1: shift: x: numeric argument required
st 2
bash: line 1: return: can only `return' from a function or sourced script
st 2
bash: line 1: return: x: numeric argument required
st 2
bash: line 1: set: nosuch: invalid option name
or
after 0
st 0
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: set: nosuch: invalid option name
after 0
st 0
bash: line 1: set: nosuch: invalid option name
or
after 0
st 0
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: set: nosuch: invalid option name
after 2 []
st 0
bash: line 1: set: nosuch: invalid option name
st 2
bash: eval: line 2: syntax error: unexpected end of file from `(' command on line 1
st 2
bash: line 1: /nonexist: No such file or directory
st 1
bash: line 1: /nonexist: No such file or directory
after 1
st 0
bash: line 1: r: readonly variable
st 127
bash: line 1: r: readonly variable
st 127
bash: line 1: r: readonly variable
st 1
bash: line 2: r: readonly variable
next 1
bash: line 4: r: readonly variable
next 1
bash: line 7: r: readonly variable
next 1
st 0
bash: line 1: `a-b': not a valid identifier
st 2
bash: line 1: r: readonly variable
st 127
1) 1
#? bash: line 1: r: readonly variable
st 127
bash: line 1: ${x!y}: bad substitution
st 127
bash: line 1: set: nosuch: invalid option name
st 2
bash: line 1: set: nosuch: invalid option name
after 2
st 0
bash: line 1: r: readonly variable
after 0
st 0
bash: line 1: `a-b': not a valid identifier
after 1
st 0

== status 0
