== stdout 2826
## ulimit -f 3; ulimit -f; ulimit -Sf; ulimit -Hf; set -o posix; ulimit -f; ulimit -Hf
3
3
3
6
6
3
3
3
6
6
## ulimit -c 5; ulimit -c; ulimit -Hc; set -o posix; ulimit -c
5
5
10
5
5
10
## set -o posix; ulimit -f 3; ulimit -f; set +o posix; ulimit -f; ulimit -a | grep -e -f
3
1
file size                   (blocks, -f) 1
3
1
file size                   (blocks, -f) 1
## set -o posix; ulimit -c 1; set +o posix; ulimit -c; ulimit -Hc
0
0
0
0
## ulimit -f 7 -c 9; ulimit -f -c; set -o posix; ulimit -c -f; ulimit -a | grep -e -c -e -f
file size                   (blocks, -f) 7
core file size              (blocks, -c) 9
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
file size                   (blocks, -f) 7
core file size              (blocks, -c) 9
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
## ulimit -S -f 7; ulimit -H -f 9; ulimit -Hf; ulimit -Sf; ulimit -f hard; ulimit -f
9
7
9
9
7
9
## ulimit -Hf 10; ulimit -f 11; echo $?; ulimit -f
bash: line 1: ulimit: file size: cannot modify limit: Invalid argument
0
11
bash: line 24: ulimit: file size: cannot modify limit: Invalid argument
0
11
## ulimit -f 9007199254740991; ulimit -f; ulimit -f 9007199254740993; echo $?; ulimit -f
9007199254740991
bash: line 1: ulimit: file size: cannot modify limit: Operation not permitted
1
9007199254740991
9007199254740991
bash: line 24: ulimit: file size: cannot modify limit: Operation not permitted
1
9007199254740991
## set -o posix; ulimit -f 18014398509481985; echo $?; ulimit -f
0
18014398509481985
0
18014398509481985
## ulimit -f 18014398509481985; echo $?; ulimit -f
bash: line 1: ulimit: 18014398509481985: limit out of range
1
unlimited
bash: line 24: ulimit: 18014398509481985: limit out of range
1
unlimited
## ulimit -t 99999999999999999999; echo $?; ulimit -t
0
7766279631452241919
0
7766279631452241919
## ulimit -s 100; ulimit -s; ulimit -d 3000; ulimit -d; ulimit -v 20000; ulimit -v
100
3000
20000
100
3000
20000
## ulimit -f 08; ulimit -f; ulimit -f 08x; ulimit -f 0x10; ulimit -f 1a; echo $?
8
bash: line 1: ulimit: 08x: invalid octal number
bash: line 1: ulimit: 0x10: invalid hex number
bash: line 1: ulimit: 1a: invalid number
1
8
bash: line 24: ulimit: 08x: invalid octal number
bash: line 24: ulimit: 0x10: invalid hex number
bash: line 24: ulimit: 1a: invalid number
1
core file size              (blocks, -c) 0
file size                   (blocks, -f) unlimited
pipe size                (512 bytes, -p) 8
core file size              (blocks, -c) 0
file size                   (blocks, -f) unlimited
pipe size                (512 bytes, -p) 8

== stderr 0

== merged 2826
## ulimit -f 3; ulimit -f; ulimit -Sf; ulimit -Hf; set -o posix; ulimit -f; ulimit -Hf
3
3
3
6
6
3
3
3
6
6
## ulimit -c 5; ulimit -c; ulimit -Hc; set -o posix; ulimit -c
5
5
10
5
5
10
## set -o posix; ulimit -f 3; ulimit -f; set +o posix; ulimit -f; ulimit -a | grep -e -f
3
1
file size                   (blocks, -f) 1
3
1
file size                   (blocks, -f) 1
## set -o posix; ulimit -c 1; set +o posix; ulimit -c; ulimit -Hc
0
0
0
0
## ulimit -f 7 -c 9; ulimit -f -c; set -o posix; ulimit -c -f; ulimit -a | grep -e -c -e -f
file size                   (blocks, -f) 7
core file size              (blocks, -c) 9
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
file size                   (blocks, -f) 7
core file size              (blocks, -c) 9
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
core file size              (blocks, -c) 18
file size                   (blocks, -f) 14
## ulimit -S -f 7; ulimit -H -f 9; ulimit -Hf; ulimit -Sf; ulimit -f hard; ulimit -f
9
7
9
9
7
9
## ulimit -Hf 10; ulimit -f 11; echo $?; ulimit -f
bash: line 1: ulimit: file size: cannot modify limit: Invalid argument
0
11
bash: line 24: ulimit: file size: cannot modify limit: Invalid argument
0
11
## ulimit -f 9007199254740991; ulimit -f; ulimit -f 9007199254740993; echo $?; ulimit -f
9007199254740991
bash: line 1: ulimit: file size: cannot modify limit: Operation not permitted
1
9007199254740991
9007199254740991
bash: line 24: ulimit: file size: cannot modify limit: Operation not permitted
1
9007199254740991
## set -o posix; ulimit -f 18014398509481985; echo $?; ulimit -f
0
18014398509481985
0
18014398509481985
## ulimit -f 18014398509481985; echo $?; ulimit -f
bash: line 1: ulimit: 18014398509481985: limit out of range
1
unlimited
bash: line 24: ulimit: 18014398509481985: limit out of range
1
unlimited
## ulimit -t 99999999999999999999; echo $?; ulimit -t
0
7766279631452241919
0
7766279631452241919
## ulimit -s 100; ulimit -s; ulimit -d 3000; ulimit -d; ulimit -v 20000; ulimit -v
100
3000
20000
100
3000
20000
## ulimit -f 08; ulimit -f; ulimit -f 08x; ulimit -f 0x10; ulimit -f 1a; echo $?
8
bash: line 1: ulimit: 08x: invalid octal number
bash: line 1: ulimit: 0x10: invalid hex number
bash: line 1: ulimit: 1a: invalid number
1
8
bash: line 24: ulimit: 08x: invalid octal number
bash: line 24: ulimit: 0x10: invalid hex number
bash: line 24: ulimit: 1a: invalid number
1
core file size              (blocks, -c) 0
file size                   (blocks, -f) unlimited
pipe size                (512 bytes, -p) 8
core file size              (blocks, -c) 0
file size                   (blocks, -f) unlimited
pipe size                (512 bytes, -p) 8

== status 0
