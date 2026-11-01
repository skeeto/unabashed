== stdout 136
init=hBc
e=ehBc
eu=ehuBc
ux=huxBc
many=abfhkuBCc
back=uBc
pipefail=uBCc
H=uBCHc
TEP=uBCEHPTc
final=uBc
sub=euBc
cs=uBc
v=uvBc
posix=uBc

== stderr 68
+ echo ux=huxBc
+ set +x
echo "v=$-" 2>/dev/null
set +v 2>/dev/null

== merged 204
init=hBc
e=ehBc
eu=ehuBc
+ echo ux=huxBc
ux=huxBc
+ set +x
many=abfhkuBCc
back=uBc
pipefail=uBCc
H=uBCHc
TEP=uBCEHPTc
final=uBc
sub=euBc
cs=uBc
echo "v=$-" 2>/dev/null
v=uvBc
set +v 2>/dev/null
posix=uBc

== status 0
