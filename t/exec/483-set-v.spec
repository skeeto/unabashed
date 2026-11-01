== stdout 15
one
2
fn
quiet

== stderr 49
echo one
x=2; echo $x
f() {
  echo fn
}
f
set +v

== merged 64
echo one
one
x=2; echo $x
2
f() {
  echo fn
}
f
fn
set +v
quiet

== status 0
