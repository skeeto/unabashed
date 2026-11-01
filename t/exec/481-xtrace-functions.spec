== stdout 36
in f: a b
st=2
in f: x
red
done
off

== stderr 171
+ f 'a b'
+ echo 'in f: a b'
+ return 2
+ echo st=2
+ g
+ f x
+ echo 'in f: x'
+ return 2
+ h
+ local v=1
+ echo 1
+ echo hi
+ echo red
+ set +x
+ : via-o
+ set +o xtrace

== merged 207
+ f 'a b'
+ echo 'in f: a b'
in f: a b
+ return 2
+ echo st=2
st=2
+ g
+ f x
+ echo 'in f: x'
in f: x
+ return 2
+ h
+ local v=1
+ echo 1
+ echo hi
+ echo red
red
+ set +x
done
+ : via-o
+ set +o xtrace
off

== status 0
