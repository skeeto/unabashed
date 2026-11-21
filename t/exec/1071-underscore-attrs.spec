== stdout 853
a b
[b]
declare -- foo="[b]"
[foo]
a b
declare -- foo="b"
unexported
x
a b
declare -- x="b"
a b
declare -a arr=([0]="x" [1]="b" [2]="z")
[]
[:]
a
in [a]
[f]
a b
in b
[in b]
declare -- _="[in b]"
a
[_=foo]
cs
{cs}
[cs
{cs}]
a b
declare -nx _="foo"
declare -- foo="b"
a b
declare -- _="b"
bash: line 1: foo: readonly variable
a b
bash: line 1: foo: readonly variable
[1]
bash: line 1: foo: readonly variable
-- readonly
x
bash: line 1: _: readonly variable
a b
bash: line 1: _: readonly variable
[x]
bash: line 1: _: readonly variable
sub [x]
bash: line 1: _: readonly variable
bash: line 1: _: readonly variable
a b
bash: line 1: _: readonly variable
[y]
bash: line 1: _: readonly variable
-- arrays
a b
declare -a _=([0]="b" [1]="2")
x
a b
declare -A _=([0]="b" )
-- case
x
abc
[ABC]
x
ABC
in [abc]
[f]
-- plain
x
a
declare -- _="a"
a b
[b]
x
abc
[abc]

== stderr 0

== merged 853
a b
[b]
declare -- foo="[b]"
[foo]
a b
declare -- foo="b"
unexported
x
a b
declare -- x="b"
a b
declare -a arr=([0]="x" [1]="b" [2]="z")
[]
[:]
a
in [a]
[f]
a b
in b
[in b]
declare -- _="[in b]"
a
[_=foo]
cs
{cs}
[cs
{cs}]
a b
declare -nx _="foo"
declare -- foo="b"
a b
declare -- _="b"
bash: line 1: foo: readonly variable
a b
bash: line 1: foo: readonly variable
[1]
bash: line 1: foo: readonly variable
-- readonly
x
bash: line 1: _: readonly variable
a b
bash: line 1: _: readonly variable
[x]
bash: line 1: _: readonly variable
sub [x]
bash: line 1: _: readonly variable
bash: line 1: _: readonly variable
a b
bash: line 1: _: readonly variable
[y]
bash: line 1: _: readonly variable
-- arrays
a b
declare -a _=([0]="b" [1]="2")
x
a b
declare -A _=([0]="b" )
-- case
x
abc
[ABC]
x
ABC
in [abc]
[f]
-- plain
x
a
declare -- _="a"
a b
[b]
x
abc
[abc]

== status 0
