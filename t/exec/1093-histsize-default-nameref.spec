== stdout 1107
== declare -n HISTFILESIZE="a[\$(cmd)]"
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
bash: line 2: declare: a: not found
== declare -n HISTSIZE="b[\$(cmd)]"; b[1]=4
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
declare -a b=([1]="4")
== declare -n HISTSIZE=zz
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
bash: line 2: declare: zz: not found
== declare -n HISTSIZE=zz; zz=7
declare -n HISTSIZE="zz"
declare -- HISTFILESIZE="7"
declare -- zz="7"
== declare -n HISTSIZE
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
== declare HISTSIZE
declare -- HISTSIZE
declare -- HISTFILESIZE
== declare -r HISTSIZE
declare -r HISTSIZE
declare -- HISTFILESIZE
== HISTSIZE=
declare -- HISTSIZE=""
declare -- HISTFILESIZE=""
== f() { local HISTSIZE; set -o history; declare -p HISTSIZE; }; f
declare -- HISTSIZE
declare -- HISTSIZE="500"
declare -- HISTFILESIZE
== f() { local -n HISTSIZE=q; set -o history; declare -p HISTSIZE q; }; f
declare -n HISTSIZE="q"
bash: line 2: declare: q: not found
declare -- HISTSIZE="500"
declare -- HISTFILESIZE
bash: line 2: declare: q: not found

== stderr 0

== merged 1107
== declare -n HISTFILESIZE="a[\$(cmd)]"
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
bash: line 2: declare: a: not found
== declare -n HISTSIZE="b[\$(cmd)]"; b[1]=4
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
declare -a b=([1]="4")
== declare -n HISTSIZE=zz
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
bash: line 2: declare: zz: not found
== declare -n HISTSIZE=zz; zz=7
declare -n HISTSIZE="zz"
declare -- HISTFILESIZE="7"
declare -- zz="7"
== declare -n HISTSIZE
declare -- HISTSIZE="500"
declare -- HISTFILESIZE="500"
== declare HISTSIZE
declare -- HISTSIZE
declare -- HISTFILESIZE
== declare -r HISTSIZE
declare -r HISTSIZE
declare -- HISTFILESIZE
== HISTSIZE=
declare -- HISTSIZE=""
declare -- HISTFILESIZE=""
== f() { local HISTSIZE; set -o history; declare -p HISTSIZE; }; f
declare -- HISTSIZE
declare -- HISTSIZE="500"
declare -- HISTFILESIZE
== f() { local -n HISTSIZE=q; set -o history; declare -p HISTSIZE q; }; f
declare -n HISTSIZE="q"
bash: line 2: declare: q: not found
declare -- HISTSIZE="500"
declare -- HISTFILESIZE
bash: line 2: declare: q: not found

== status 1
