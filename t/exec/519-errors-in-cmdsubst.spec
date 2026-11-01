== stdout 88
1 st=1 x=a
2 st=1 x=a
3 st=1 x=a
4 st=1 x=a
5 st=0 x=b
6 st=1 x=
7 st=127
8 [] st=1
end

== stderr 324
bash: line 1: ${bad x}: bad substitution
bash: line 2: 1/0: division by 0 (error token is "0")
bash: line 3: nope: unbound variable
bash: line 4: nope: gone
bash: line 5: exit: abc: numeric argument required
bash: line 6: r: readonly variable
bash: line 7: nosuch: command not found
bash: line 8: ${bad x}: bad substitution

== merged 412
bash: line 1: ${bad x}: bad substitution
1 st=1 x=a
bash: line 2: 1/0: division by 0 (error token is "0")
2 st=1 x=a
bash: line 3: nope: unbound variable
3 st=1 x=a
bash: line 4: nope: gone
4 st=1 x=a
bash: line 5: exit: abc: numeric argument required
5 st=0 x=b
bash: line 6: r: readonly variable
6 st=1 x=
bash: line 7: nosuch: command not found
7 st=127
bash: line 8: ${bad x}: bad substitution
8 [] st=1
end

== status 0
