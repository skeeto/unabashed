== stdout 257
a*: Alpha
*TXT: Echo.txt delta.TXT
[a-c]*: Alpha beta
[[:upper:]]*: Alpha Dir Echo.txt GAMMA
[[:lower:]]*: beta delta.TXT
?AMMA: GAMMA
dir/*: dir/*
D*/f*: Dir/File
[!a]*: Dir Echo.txt GAMMA beta delta.TXT
@(alpha|echo)*: Alpha Echo.txt
off a*: a*
case kept

== stderr 0

== merged 257
a*: Alpha
*TXT: Echo.txt delta.TXT
[a-c]*: Alpha beta
[[:upper:]]*: Alpha Dir Echo.txt GAMMA
[[:lower:]]*: beta delta.TXT
?AMMA: GAMMA
dir/*: dir/*
D*/f*: Dir/File
[!a]*: Dir Echo.txt GAMMA beta delta.TXT
@(alpha|echo)*: Alpha Echo.txt
off a*: a*
case kept

== status 0
