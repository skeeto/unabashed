== stdout 258
n=2 1=outer1 2=outer2 all=outer1 outer2
n=1 1=new1 2=none all=new1
after: 2 outer1 outer2
n=3 1=a 2=b all=a b c
after2: 2 outer1 outer2
n=2 1=fa 2=fb all=fa fb
n=1 1=z 2=none all=z
f: fa fb
shifted: outer2
outer now: replaced
shifted: y
outer now2: replaced

== stderr 0

== merged 258
n=2 1=outer1 2=outer2 all=outer1 outer2
n=1 1=new1 2=none all=new1
after: 2 outer1 outer2
n=3 1=a 2=b all=a b c
after2: 2 outer1 outer2
n=2 1=fa 2=fb all=fa fb
n=1 1=z 2=none all=z
f: fa fb
shifted: outer2
outer now: replaced
shifted: y
outer now2: replaced

== status 0
