== stdout 95
a 0 0
b 0 b a
c 0 c b a
e 0 k
f 0 *
g 0 k a
h 0 k
i 0 k a <>
j 0 1 p j
k 0 p q
l 0 a [k]=v
end

== stderr 533
bash: line 9: '': bad array subscript
bash: line 10: '': bad array subscript
bash: line 11: '': bad array subscript
bash: line 12: []=$(echo notexpanded 1>&2): bad array subscript
bash: line 14: h: [k]: must use subscript when assigning associative array
bash: line 15: h: [k]: must use subscript when assigning associative array
expanded
bash: line 19: '': bad array subscript
bash: line 22: '': bad array subscript
+ h4=('a' 'k' 'v')
+ declare h4
+ h4=(['p q']='p q' ['']='1')
bash: line 26: ['']='1': bad array subscript
+ set +x

== merged 628
bash: line 9: '': bad array subscript
a 0 0
bash: line 10: '': bad array subscript
b 0 b a
bash: line 11: '': bad array subscript
c 0 c b a
bash: line 12: []=$(echo notexpanded 1>&2): bad array subscript
e 0 k
bash: line 14: h: [k]: must use subscript when assigning associative array
bash: line 15: h: [k]: must use subscript when assigning associative array
f 0 *
g 0 k a
expanded
bash: line 19: '': bad array subscript
h 0 k
i 0 k a <>
j 0 1 p j
bash: line 22: '': bad array subscript
k 0 p q
l 0 a [k]=v
+ h4=('a' 'k' 'v')
+ declare h4
+ h4=(['p q']='p q' ['']='1')
bash: line 26: ['']='1': bad array subscript
+ set +x
end

== status 0
