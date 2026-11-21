== stdout 889
fn 1
declare -a d=([0]="a" [1]="2" [2]="more" [3]="again")
declare -a d2=([0]="a" [1]="b c" [2]="2")
declare -- e="1"
declare -A h=([l]="m n" [k]="v" )
declare -A h2=([l]="m n" [k]="v" )
declare -a g=([0]="3")
declare -ax ex=([0]="1")
declare -ar rx=([0]="1" [1]="2")
declare -ai n=([0]="3" [1]="2")
declare -a t=()
declare -a dd=([0]="x")
declare -- q="(a b)"
declare -a w=([1]="a" [3]="b")
declare -a b=([0]="x" [2]="y" [5]="z" [6]="w")
declare -a c=([0]="1")
declare -- y="1"
declare -a a=([0]="p" [1]="q")
declare -- z="2"
declare -A ha=([b]="2" [a]="1" )
declare -ar r1=([0]="1")
declare -r r2="x"
declare -a aa=([0]="old")
declare -A h3=()
declare -A h4=([k]="v" )
declare -a i=()
declare -a i2=()
declare -A h5=([g]="7" [f]="" [e]="5" [c]="" [b]="2" [a]="1" )
declare -A h6=([a]="1" )
declare -a i3=()
declare -a i4=([3]="1" [5]="2" [7]="5")
declare -a i5=([3]="1" [5]="2" [7]="5")

== stderr 1660
+ x=2
+ d=('a' '2')
+ declare -a d
+ d2=('a' 'b c' '2')
+ declare d2 e=1
+ h=(['k']='v' ['l']='m n')
+ declare -A h
+ h2=('k' 'v' 'l' 'm n')
+ declare -A h2
+ f
+ l=('1' '2')
+ local l
+ g=('3')
+ declare -g g
+ ex=('1')
+ export ex
+ rx=('1' '2')
+ readonly rx
+ n=('1+1' '2')
+ declare -i n
+ d+=(more)
+ d+=('again')
+ declare d
+ t=()
+ typeset -a t
+ dd=('x')
+ declare -- dd
+ declare 'q=(a b)'
+ w=(['1']='a' ['3']='b')
+ declare -a w
+ b=(x)
+ b+=(['2']+='y' ['5']='z' 'w')
+ declare b
+ c=('1')
+ X=1
+ declare c
+ a=('p' 'q')
+ declare y=1 a z=2
+ declare -A ha
+ ha+=(['a']='1' ['b']+='2')
+ declare ha
+ r1=('1')
+ declare -r r1 r2=x
+ x=old
+ aa=('old')
+ declare x=new aa
+ h3=(['']='1' ['k']='v')
bash: line 33: ['']='1': bad array subscript
+ h4=('k' 'v' '' 'w')
bash: line 34: '': bad array subscript
+ declare -A h4
+ i=(['']='1' ['2']='b')
bash: line 35: []=1: bad array subscript
+ i2=(['-5']='1' 'a')
bash: line 36: [-5]=1: bad array subscript
+ h5=(['a']='1')
+ declare -A h5
+ h5+=(['b']='2' ['c']='' ['']='3' ['d']='4')
bash: line 38: ['']='3': bad array subscript
+ h5+=('e' '5' 'f' '' '' '6' 'g' '7')
bash: line 39: '': bad array subscript
+ declare h5
+ h6=(['a']='1' 'b')
bash: line 40: h6: 'b': must use subscript when assigning associative array
+ i3=(['@']='1' 'b')
bash: line 41: [@]=1: cannot assign to non-numeric index
+ n=5
+ i4=(['n=3']='1' ['5']='2' ['7']='5')
+ declare -a i4
+ fn
+ xx=('1')
+ declare -n nn=x xx yy=1bad
bash: line 44: declare: xx: reference variable cannot be an array
bash: line 44: declare: `1bad': invalid variable name for name reference
+ echo 'fn 1'
+ set +x
bash: line 48: declare: xx: not found

== merged 2549
+ x=2
+ d=('a' '2')
+ declare -a d
+ d2=('a' 'b c' '2')
+ declare d2 e=1
+ h=(['k']='v' ['l']='m n')
+ declare -A h
+ h2=('k' 'v' 'l' 'm n')
+ declare -A h2
+ f
+ l=('1' '2')
+ local l
+ g=('3')
+ declare -g g
+ ex=('1')
+ export ex
+ rx=('1' '2')
+ readonly rx
+ n=('1+1' '2')
+ declare -i n
+ d+=(more)
+ d+=('again')
+ declare d
+ t=()
+ typeset -a t
+ dd=('x')
+ declare -- dd
+ declare 'q=(a b)'
+ w=(['1']='a' ['3']='b')
+ declare -a w
+ b=(x)
+ b+=(['2']+='y' ['5']='z' 'w')
+ declare b
+ c=('1')
+ X=1
+ declare c
+ a=('p' 'q')
+ declare y=1 a z=2
+ declare -A ha
+ ha+=(['a']='1' ['b']+='2')
+ declare ha
+ r1=('1')
+ declare -r r1 r2=x
+ x=old
+ aa=('old')
+ declare x=new aa
+ h3=(['']='1' ['k']='v')
bash: line 33: ['']='1': bad array subscript
+ h4=('k' 'v' '' 'w')
bash: line 34: '': bad array subscript
+ declare -A h4
+ i=(['']='1' ['2']='b')
bash: line 35: []=1: bad array subscript
+ i2=(['-5']='1' 'a')
bash: line 36: [-5]=1: bad array subscript
+ h5=(['a']='1')
+ declare -A h5
+ h5+=(['b']='2' ['c']='' ['']='3' ['d']='4')
bash: line 38: ['']='3': bad array subscript
+ h5+=('e' '5' 'f' '' '' '6' 'g' '7')
bash: line 39: '': bad array subscript
+ declare h5
+ h6=(['a']='1' 'b')
bash: line 40: h6: 'b': must use subscript when assigning associative array
+ i3=(['@']='1' 'b')
bash: line 41: [@]=1: cannot assign to non-numeric index
+ n=5
+ i4=(['n=3']='1' ['5']='2' ['7']='5')
+ declare -a i4
+ fn
+ xx=('1')
+ declare -n nn=x xx yy=1bad
bash: line 44: declare: xx: reference variable cannot be an array
bash: line 44: declare: `1bad': invalid variable name for name reference
+ echo 'fn 1'
fn 1
+ set +x
declare -a d=([0]="a" [1]="2" [2]="more" [3]="again")
declare -a d2=([0]="a" [1]="b c" [2]="2")
declare -- e="1"
declare -A h=([l]="m n" [k]="v" )
declare -A h2=([l]="m n" [k]="v" )
declare -a g=([0]="3")
declare -ax ex=([0]="1")
declare -ar rx=([0]="1" [1]="2")
declare -ai n=([0]="3" [1]="2")
declare -a t=()
declare -a dd=([0]="x")
declare -- q="(a b)"
declare -a w=([1]="a" [3]="b")
declare -a b=([0]="x" [2]="y" [5]="z" [6]="w")
declare -a c=([0]="1")
declare -- y="1"
declare -a a=([0]="p" [1]="q")
declare -- z="2"
declare -A ha=([b]="2" [a]="1" )
declare -ar r1=([0]="1")
declare -r r2="x"
declare -a aa=([0]="old")
declare -A h3=()
declare -A h4=([k]="v" )
declare -a i=()
declare -a i2=()
declare -A h5=([g]="7" [f]="" [e]="5" [c]="" [b]="2" [a]="1" )
declare -A h6=([a]="1" )
declare -a i3=()
declare -a i4=([3]="1" [5]="2" [7]="5")
bash: line 48: declare: xx: not found
declare -a i5=([3]="1" [5]="2" [7]="5")

== status 0
