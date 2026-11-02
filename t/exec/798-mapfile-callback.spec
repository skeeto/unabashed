== stdout 778
cb 0 x

cb 1 y

declare -a a=([0]=$'x\n' [1]=$'y\n')
cb 0 x
cb 1 y
declare -a a=([0]="x" [1]="y")
[1 2
]
[3 4
]
0
declare -a a=([0]=$'1\n' [1]=$'2\n' [2]=$'3\n' [3]=$'4\n' [4]=$'5\n')
<0>
<it's $HOME>
bash: stdin: line 6: syntax error near unexpected token `0'
bash: stdin: line 6: `f( 0 'x'
0
foo 0 1

foo 1 2

foo 2 3

foo 3 4

foo 4 5

callback 0 read 2
callback 1 read 4
callback 2 read 6
callback 3 read 8
callback 4 read 10
callback 5 read 12
callback 6 read 14
callback 7 read 16
callback 8 read 18
callback 9 read 20
declare -a a=([0]=$'1\n' [1]=$'3\n' [2]=$'5\n' [3]=$'7\n' [4]=$'9\n' [5]=$'11\n' [6]=$'13\n' [7]=$'15\n' [8]=$'17\n' [9]=$'19\n')
callback 1 read 3
callback 3 read 6
declare -a a=([0]="1" [1]="2" [2]="4" [3]="5")
0 abc
1 def
2 ghi
3 jkl
abc def ghi jkl

== stderr 0

== merged 778
cb 0 x

cb 1 y

declare -a a=([0]=$'x\n' [1]=$'y\n')
cb 0 x
cb 1 y
declare -a a=([0]="x" [1]="y")
[1 2
]
[3 4
]
0
declare -a a=([0]=$'1\n' [1]=$'2\n' [2]=$'3\n' [3]=$'4\n' [4]=$'5\n')
<0>
<it's $HOME>
bash: stdin: line 6: syntax error near unexpected token `0'
bash: stdin: line 6: `f( 0 'x'
0
foo 0 1

foo 1 2

foo 2 3

foo 3 4

foo 4 5

callback 0 read 2
callback 1 read 4
callback 2 read 6
callback 3 read 8
callback 4 read 10
callback 5 read 12
callback 6 read 14
callback 7 read 16
callback 8 read 18
callback 9 read 20
declare -a a=([0]=$'1\n' [1]=$'3\n' [2]=$'5\n' [3]=$'7\n' [4]=$'9\n' [5]=$'11\n' [6]=$'13\n' [7]=$'15\n' [8]=$'17\n' [9]=$'19\n')
callback 1 read 3
callback 3 read 6
declare -a a=([0]="1" [1]="2" [2]="4" [3]="5")
0 abc
1 def
2 ghi
3 jkl
abc def ghi jkl

== status 0
