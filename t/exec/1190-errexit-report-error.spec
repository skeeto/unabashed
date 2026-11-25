== stdout 2102
-- echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- u=; echo ${!u}
bash: line 1: : invalid variable name
status 1
-- u=1a; echo ${!u}
bash: line 1: 1a: invalid variable name
status 1
-- echo ${!nosuch}
bash: line 1: nosuch: invalid indirect expansion
status 3
-- x=(1); echo ${x[-5]}
bash: line 1: x: bad array subscript
status 1
-- x=(1); echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
status 1
-- echo ${arr[-1]:=a}
bash: line 1: arr: bad array subscript
status 3
-- echo ${1=x}
bash: line 1: $1: cannot assign in this way
status 1
-- shopt -s failglob; echo /nope*
bash: line 1: no match: /nope*
status 1
-- a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- x=1 a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- a=([]=1)
bash: line 1: []=1: bad array subscript
status 3
-- x=(1); x[-5]+=1
bash: line 1: x[-5]: bad array subscript
status 1
-- declare a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- declare -A m; m[]=1
bash: line 1: m[]: bad array subscript
status 1
-- a=(1); a[0]=(2)
bash: line 1: a[0]: cannot assign list to array member
status 1
-- echo $(( a[] ))
bash: line 1: a[]: bad array subscript
status 3
-- set -u; echo ${a[@]}${a[1]}
bash: line 1: a[1]: unbound variable
status 1
-- echo `echo a\`
bash: -c: line 1: unexpected EOF while looking for matching ``'
status 2
-- echo ${x!} && echo and
bash: line 1: ${x!}: bad substitution
status 1
-- if echo ${x!}; then :; fi
bash: line 1: ${x!}: bad substitution
status 1
-- ( echo ${x!}; echo in ); echo "sub $?"
bash: line 1: ${x!}: bad substitution
status 1
-- y=$(echo ${x!}; echo in); echo "comsub <$y>"
bash: line 1: ${x!}: bad substitution
status 1
-- y=$(set -e; echo ${x!}; echo in); echo "comsub -e <$y>"
bash: line 1: ${x!}: bad substitution
status 1
-- if eval 'echo ${x!}'; then :; fi
bash: line 1: ${x!}: bad substitution
after
status 0
-- echo $((1/0))
bash: line 1: 1/0: division by 0 (error token is "0")
after
status 0
-- x=abc; echo ${x:1:-5}
bash: line 1: -5: substring expression < 0
after
status 0
-- set +e; echo ${x!}
bash: line 1: ${x!}: bad substitution
after
status 0

== stderr 0

== merged 2102
-- echo ${x!}
bash: line 1: ${x!}: bad substitution
status 1
-- u=; echo ${!u}
bash: line 1: : invalid variable name
status 1
-- u=1a; echo ${!u}
bash: line 1: 1a: invalid variable name
status 1
-- echo ${!nosuch}
bash: line 1: nosuch: invalid indirect expansion
status 3
-- x=(1); echo ${x[-5]}
bash: line 1: x: bad array subscript
status 1
-- x=(1); echo ${#x[-5]}
bash: line 1: [-5]: bad array subscript
status 1
-- echo ${arr[-1]:=a}
bash: line 1: arr: bad array subscript
status 3
-- echo ${1=x}
bash: line 1: $1: cannot assign in this way
status 1
-- shopt -s failglob; echo /nope*
bash: line 1: no match: /nope*
status 1
-- a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- x=1 a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- a=([]=1)
bash: line 1: []=1: bad array subscript
status 3
-- x=(1); x[-5]+=1
bash: line 1: x[-5]: bad array subscript
status 1
-- declare a[]=1
bash: line 1: a[]: bad array subscript
status 3
-- declare -A m; m[]=1
bash: line 1: m[]: bad array subscript
status 1
-- a=(1); a[0]=(2)
bash: line 1: a[0]: cannot assign list to array member
status 1
-- echo $(( a[] ))
bash: line 1: a[]: bad array subscript
status 3
-- set -u; echo ${a[@]}${a[1]}
bash: line 1: a[1]: unbound variable
status 1
-- echo `echo a\`
bash: -c: line 1: unexpected EOF while looking for matching ``'
status 2
-- echo ${x!} && echo and
bash: line 1: ${x!}: bad substitution
status 1
-- if echo ${x!}; then :; fi
bash: line 1: ${x!}: bad substitution
status 1
-- ( echo ${x!}; echo in ); echo "sub $?"
bash: line 1: ${x!}: bad substitution
status 1
-- y=$(echo ${x!}; echo in); echo "comsub <$y>"
bash: line 1: ${x!}: bad substitution
status 1
-- y=$(set -e; echo ${x!}; echo in); echo "comsub -e <$y>"
bash: line 1: ${x!}: bad substitution
status 1
-- if eval 'echo ${x!}'; then :; fi
bash: line 1: ${x!}: bad substitution
after
status 0
-- echo $((1/0))
bash: line 1: 1/0: division by 0 (error token is "0")
after
status 0
-- x=abc; echo ${x:1:-5}
bash: line 1: -5: substring expression < 0
after
status 0
-- set +e; echo ${x!}
bash: line 1: ${x!}: bad substitution
after
status 0

== status 0
