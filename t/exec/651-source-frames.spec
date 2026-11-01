== stdout 1159
top: FUNCNAME=unset SRC= LN=
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=11
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 11
3 source ./sub.sh
caller: 
in sub: FUNCNAME=source f #2 SRC=./sub.sh bash LINENO=12 13
sf: FUNCNAME=sf source f SRC=./sub.sh ./sub.sh bash LN=3 12 13
3 source ./sub.sh
12 f bash
caller: 12 f bash
---- extdebug
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=16
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 16
3 source ./sub.sh
caller: 
in sub: FUNCNAME=source f #2 SRC=./sub.sh bash LINENO=12 17
sf: FUNCNAME=sf source f SRC=./sub.sh ./sub.sh bash LN=3 12 17
3 source ./sub.sh
12 f bash
caller: 12 f bash
debug 19 none 
debug 1 none ./sub.sh
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=19
debug 3 none ./sub.sh
debug 2 sf ./sub.sh
debug 2 sf ./sub.sh
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 19
debug 2 sf ./sub.sh
3 source ./sub.sh
debug 2 sf ./sub.sh
debug 2 sf ./sub.sh
debug 4 none ./sub.sh
caller: debug 4 none ./sub.sh
debug 20 none 
---- argv
argc=2 2 argv=2 1 two one
s2 argc=3 2 2 argv=z y x 2 1 two one
after argc=2 2 argv=2 1 two one
argc=2 argv=two one
s2 argc=2 argv=two one
after argc=2 argv=two one

== stderr 0

== merged 1159
top: FUNCNAME=unset SRC= LN=
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=11
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 11
3 source ./sub.sh
caller: 
in sub: FUNCNAME=source f #2 SRC=./sub.sh bash LINENO=12 13
sf: FUNCNAME=sf source f SRC=./sub.sh ./sub.sh bash LN=3 12 13
3 source ./sub.sh
12 f bash
caller: 12 f bash
---- extdebug
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=16
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 16
3 source ./sub.sh
caller: 
in sub: FUNCNAME=source f #2 SRC=./sub.sh bash LINENO=12 17
sf: FUNCNAME=sf source f SRC=./sub.sh ./sub.sh bash LN=3 12 17
3 source ./sub.sh
12 f bash
caller: 12 f bash
debug 19 none 
debug 1 none ./sub.sh
in sub: FUNCNAME=unset #0 SRC=./sub.sh LINENO=19
debug 3 none ./sub.sh
debug 2 sf ./sub.sh
debug 2 sf ./sub.sh
sf: FUNCNAME=sf source SRC=./sub.sh ./sub.sh LN=3 19
debug 2 sf ./sub.sh
3 source ./sub.sh
debug 2 sf ./sub.sh
debug 2 sf ./sub.sh
debug 4 none ./sub.sh
caller: debug 4 none ./sub.sh
debug 20 none 
---- argv
argc=2 2 argv=2 1 two one
s2 argc=3 2 2 argv=z y x 2 1 two one
after argc=2 2 argv=2 1 two one
argc=2 argv=two one
s2 argc=2 argv=two one
after argc=2 argv=two one

== status 0
