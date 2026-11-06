== stdout 312
r 0
    1  old1
    2  old2
    3  old3
r 1
r 1
a
b
    1  x1
    2  x2
x1
x2
--
x1
x2
--
    1  100: invalid timestampstamped
    2  200: invalid timestampmulti
    3  NOW: invalid timestampline
    4  300: invalid timestamplast
stamped
multi
line
last
--
4
5
0
a 1
w 1
a 1
a 0
hi
trap
old
--
a
b
old1
old2
ext

== stderr 156
bash: line 1: history: HISTFILE: parameter null or not set
bash: line 1: history: empty filename
bash: line 1: history: HISTFILE: parameter null or not set

== merged 468
r 0
    1  old1
    2  old2
    3  old3
r 1
r 1
a
b
    1  x1
    2  x2
x1
x2
--
x1
x2
--
    1  100: invalid timestampstamped
    2  200: invalid timestampmulti
    3  NOW: invalid timestampline
    4  300: invalid timestamplast
stamped
multi
line
last
--
4
5
0
bash: line 1: history: HISTFILE: parameter null or not set
a 1
bash: line 1: history: empty filename
w 1
bash: line 1: history: HISTFILE: parameter null or not set
a 1
a 0
hi
trap
old
--
a
b
old1
old2
ext

== status 0
