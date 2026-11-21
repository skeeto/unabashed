== stdout 26
a
b
c
d
e
f
g
h
i
j
k
end

== stderr 1593
bash: line 8: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 8: /nonexistent/x: No such file or directory
bash: line 10: ro: readonly variable
bash: line 11: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
+ LC_ALL=y
+ :
+ set +x
bash: line 12: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
bash: line 13: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 13: /nonexistent/x: No such file or directory
bash: line 13: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
bash: line 14: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
bash: line 15: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 15: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
bash: line 16: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
1 y
bash: line 1: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
2 x
bash: line 18: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
f1 y
f2 y
bash: line 19: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
e1 y
e2 y
bash: line 20: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 20: /nonexistent/x: No such file or directory
bash: line 20: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
w
<1>

== merged 1619
a
bash: line 8: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 8: /nonexistent/x: No such file or directory
b
bash: line 10: ro: readonly variable
c
bash: line 11: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
+ LC_ALL=y
+ :
+ set +x
bash: line 12: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
d
bash: line 13: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 13: /nonexistent/x: No such file or directory
bash: line 13: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
e
bash: line 14: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
f
bash: line 15: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 15: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
g
bash: line 16: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
1 y
bash: line 1: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
2 x
h
bash: line 18: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
f1 y
f2 y
i
bash: line 19: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
e1 y
e2 y
j
bash: line 20: warning: setlocale: LC_ALL: cannot change locale (y): No such file or directory
bash: line 20: /nonexistent/x: No such file or directory
bash: line 20: warning: setlocale: LC_ALL: cannot change locale (x): No such file or directory
k
w
<1>
end

== status 0
