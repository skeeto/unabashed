== stdout 301
bash: line 4: cd: $'no\tsuch': No such file or directory
bash: line 5: no	file: No such file or directory
builtin hash -p '/a b/c' 'x y'
builtin hash -p /a b/c x y
bash: line 9: hash: x y: not found
1
bash: line 10: hash: x y: not found
[1970-01-02 0            ]
[                     19:00 EST]
0
2

== stderr 0

== merged 301
bash: line 4: cd: $'no\tsuch': No such file or directory
bash: line 5: no	file: No such file or directory
builtin hash -p '/a b/c' 'x y'
builtin hash -p /a b/c x y
bash: line 9: hash: x y: not found
1
bash: line 10: hash: x y: not found
[1970-01-02 0            ]
[                     19:00 EST]
0
2

== status 0
