== stdout 591
[éà]
üx
[éàü]
x
[éà]
[éà]
 ff fe
ab
[é] 0
 c3
 c3 a9
àüx
[ a b  c][]
[ac]
[anb]
[ab
c] 0
[ab] 0
[ab] 1
declare -a arr=([0]="a" [1]="b")
declare -a arr=([0]="  a  b  ")
declare -a arr=([0]="a" [1]="b" [2]="" [3]="c")
[a][b][:c::]
[a][b][:c::]
[one][two]
   a   b
   x   0   y
[] 0
[] 0
0
[abc]
[abc]
[abc] 0
1
1
1
1
1
bash: line 38: read: -d: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
2
2
1
1 [x]
1
0 x
declare -a a=([0]="x" [1]="x" [2]="p" [3]="q")
declare -a a=()

== stderr 588
bash: line 33: read: -1: invalid timeout specification
bash: line 34: read: 1.5x: invalid timeout specification
bash: line 35: read: -1: invalid number
bash: line 36: read: 1x: invalid number
bash: line 37: read: 1x: invalid file descriptor specification
bash: line 39: read: -a: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
bash: line 40: read: `a b': not a valid identifier
bash: line 41: read: `b c': not a valid identifier
bash: line 42: read: `a[1]': not a valid identifier

== merged 1179
[éà]
üx
[éàü]
x
[éà]
[éà]
 ff fe
ab
[é] 0
 c3
 c3 a9
àüx
[ a b  c][]
[ac]
[anb]
[ab
c] 0
[ab] 0
[ab] 1
declare -a arr=([0]="a" [1]="b")
declare -a arr=([0]="  a  b  ")
declare -a arr=([0]="a" [1]="b" [2]="" [3]="c")
[a][b][:c::]
[a][b][:c::]
[one][two]
   a   b
   x   0   y
[] 0
[] 0
0
[abc]
[abc]
[abc] 0
bash: line 33: read: -1: invalid timeout specification
1
bash: line 34: read: 1.5x: invalid timeout specification
1
bash: line 35: read: -1: invalid number
1
bash: line 36: read: 1x: invalid number
1
bash: line 37: read: 1x: invalid file descriptor specification
1
bash: line 38: read: -d: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
2
bash: line 39: read: -a: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
2
bash: line 40: read: `a b': not a valid identifier
1
bash: line 41: read: `b c': not a valid identifier
1 [x]
bash: line 42: read: `a[1]': not a valid identifier
1
0 x
declare -a a=([0]="x" [1]="x" [2]="p" [3]="q")
declare -a a=()

== status 0
