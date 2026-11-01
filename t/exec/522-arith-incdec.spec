== stdout 210
i++ = 5 now 6
++i = 7 now 7
i-- = 7 now 6
--i = 5 now 5
j++ + j++ = 3 now 3
++k * ++k = 6 now 3
m++ = 0 now 1
--m = -1 now -1
s++ = 7 now 8
e now 8
- -x = 10 x=10
-+x = -10
+ +x = 10
x--- = 9 x=9
z+++z = 5 z=3

== stderr 0

== merged 210
i++ = 5 now 6
++i = 7 now 7
i-- = 7 now 6
--i = 5 now 5
j++ + j++ = 3 now 3
++k * ++k = 6 now 3
m++ = 0 now 1
--m = -1 now -1
s++ = 7 now 8
e now 8
- -x = 10 x=10
-+x = -10
+ +x = 10
x--- = 9 x=9
z+++z = 5 z=3

== status 0
