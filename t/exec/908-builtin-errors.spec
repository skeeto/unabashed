== stdout 2186
s1: line 1: shift: too many arguments
next 2
s1: line 3: return: too many arguments
next 2
s1: line 6: return: too many arguments
next 2
bash: line 1: shift: too many arguments
st 1
bash: line 1: exit: too many arguments
cs 1
bash: line 1: exit: too many arguments
sub 1
bash: line 15: alias: -x: invalid option
alias: usage: alias [-p] [name[=value] ... ]
2
bash: line 16: alias: -x: invalid option
alias: usage: alias [-p] [name[=value] ... ]
2
bash: line 17: unalias: -x: invalid option
unalias: usage: unalias [-a] name [name ...]
2
bash: line 18: unalias: -x: invalid option
unalias: usage: unalias [-a] name [name ...]
2
bash: line 19: unalias: --: invalid option
unalias: usage: unalias [-a] name [name ...]
2
alias a='b'
bash: line 21: readonly: -x: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
2
bash: line 22: readonly: -i: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
2
v=1 0
0
bash: line 25: builtin: -x: invalid option
builtin: usage: builtin [shell-builtin [arg ...]]
2
builtin
bash: line 28: declare: -a: invalid option
1
bash: line 29: declare: -i: invalid option
1
bash: line 30: declare: -n: invalid option
1
0
0
bash: line 33: declare: cannot use `-f' to make functions
1
bash: line 34: declare: x=1: not found
1
f () 
{ 
    :
}
f () 
{ 
    :
}
declare -fx f
bash: line 36: declare: f: readonly function
1
bash: line 37: declare: f: readonly function
1
bash: line 38: hash: -d: option requires an argument
1
bash: line 39: hash: -t: option requires an argument
1
bash: line 40: hash: hashing disabled
1
bash: line 40: hash: hashing disabled
1
bash: line 1: PWD: readonly variable
1 oracle
d
bash: line 1: OLDPWD: readonly variable
1 d
bash: line 1: unset: BASH_ARGC: cannot unset
BASH_ARGC 1
bash: line 1: unset: BASH_ARGV: cannot unset
BASH_ARGV 1
bash: line 1: unset: BASH_LINENO: cannot unset
BASH_LINENO 1
bash: line 1: unset: BASH_SOURCE: cannot unset
BASH_SOURCE 1
FUNCNAME 0
bash: line 1: unset: BASH_SOURCE: cannot unset
st 1
s2: line 2: `$1': not a valid identifier
s2: line 8: `$1': not a valid identifier
s2: line 15: `$1': not a valid identifier
s3: line 6: h: readonly function

== stderr 0

== merged 2186
s1: line 1: shift: too many arguments
next 2
s1: line 3: return: too many arguments
next 2
s1: line 6: return: too many arguments
next 2
bash: line 1: shift: too many arguments
st 1
bash: line 1: exit: too many arguments
cs 1
bash: line 1: exit: too many arguments
sub 1
bash: line 15: alias: -x: invalid option
alias: usage: alias [-p] [name[=value] ... ]
2
bash: line 16: alias: -x: invalid option
alias: usage: alias [-p] [name[=value] ... ]
2
bash: line 17: unalias: -x: invalid option
unalias: usage: unalias [-a] name [name ...]
2
bash: line 18: unalias: -x: invalid option
unalias: usage: unalias [-a] name [name ...]
2
bash: line 19: unalias: --: invalid option
unalias: usage: unalias [-a] name [name ...]
2
alias a='b'
bash: line 21: readonly: -x: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
2
bash: line 22: readonly: -i: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
2
v=1 0
0
bash: line 25: builtin: -x: invalid option
builtin: usage: builtin [shell-builtin [arg ...]]
2
builtin
bash: line 28: declare: -a: invalid option
1
bash: line 29: declare: -i: invalid option
1
bash: line 30: declare: -n: invalid option
1
0
0
bash: line 33: declare: cannot use `-f' to make functions
1
bash: line 34: declare: x=1: not found
1
f () 
{ 
    :
}
f () 
{ 
    :
}
declare -fx f
bash: line 36: declare: f: readonly function
1
bash: line 37: declare: f: readonly function
1
bash: line 38: hash: -d: option requires an argument
1
bash: line 39: hash: -t: option requires an argument
1
bash: line 40: hash: hashing disabled
1
bash: line 40: hash: hashing disabled
1
bash: line 1: PWD: readonly variable
1 oracle
d
bash: line 1: OLDPWD: readonly variable
1 d
bash: line 1: unset: BASH_ARGC: cannot unset
BASH_ARGC 1
bash: line 1: unset: BASH_ARGV: cannot unset
BASH_ARGV 1
bash: line 1: unset: BASH_LINENO: cannot unset
BASH_LINENO 1
bash: line 1: unset: BASH_SOURCE: cannot unset
BASH_SOURCE 1
FUNCNAME 0
bash: line 1: unset: BASH_SOURCE: cannot unset
st 1
s2: line 2: `$1': not a valid identifier
s2: line 8: `$1': not a valid identifier
s2: line 15: `$1': not a valid identifier
s3: line 6: h: readonly function

== status 1
