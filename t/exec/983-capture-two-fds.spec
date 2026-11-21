== stdout 173
1: out=[to-stdout
err=[to-stderr]]
2: before,o1,o2,err=[e1],after
3: outer,y=[inner]
4: to-a,b=[to-b,c=[to-c]]
5: cat: /nonexistent: No such file or directory,b=[st 1,c=[]]

== stderr 0

== merged 173
1: out=[to-stdout
err=[to-stderr]]
2: before,o1,o2,err=[e1],after
3: outer,y=[inner]
4: to-a,b=[to-b,c=[to-c]]
5: cat: /nonexistent: No such file or directory,b=[st 1,c=[]]

== status 0
