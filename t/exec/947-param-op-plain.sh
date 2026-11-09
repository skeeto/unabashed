# ${x-w} ${x:-w} ${x+w} ${x:+w} with a word of plain text, as command
# words: quoted and not, on every kind of value, splitting and globbing
# the result, under set -u
p() { printf '%s' "$#"; printf ' <%s>' "$@"; echo; }
e=; s=val; sp='a b'; g='*.zz'; unset u
declare -a arr=(x y) holey; holey[3]=h
declare -A assoc=([0]=z [k]=v)
declare -i n=5
declare -n ref=s
cd /tmp && : >m1.zz && : >m2.zz
for var in e s sp g u arr holey assoc n ref; do
    echo "== $var"
    eval 'p ${'$var'-d} ${'$var':-d} ${'$var'+d} ${'$var':+d}'
    eval 'p "${'$var'-d}" "${'$var':-d}" "${'$var'+d}" "${'$var':+d}"'
    eval 'p ${'$var'-} ${'$var':-} ${'$var'+} ${'$var':+}'
    eval 'p "${'$var'-}" "${'$var':-}" "${'$var'+}" "${'$var':+}"'
    eval 'p ${'$var':-a b} ${'$var':+a b} "${'$var':-a b}"'
    eval 'p ${'$var':-*.zz} ${'$var':+m?.zz} "${'$var':-*.zz}"'
    eval 'p ${'$var':-a:b} ${'$var':-a=b,c} ${'$var':-x/y.z@w%q^r}'
done
IFS=:; p ${u:-a:b} ${s:+c:d} "${u:-a:b}"; unset IFS; p ${u:-a b}; IFS=
p ${u:-a b} ${u:+x}; IFS=$' \t\n'
set -u
p ${u-d} ${u:-} "${u:+x}" "${u+}"
( p $u ) 2>/dev/null || echo "unbound $?"
set +u
f() { local u=in; p ${u:-d}; }; f; p ${u:-d}
p2() { p ${u:-d}; }; u=t p2
for i in 1 2; do p ${i:+set$i} ${j:-none}; j=$i; done
