== stdout 197
*/sub: d1/sub d2/sub d3/sub
*/sub/*: d1/sub/f d2/sub/g
locked/*: locked/*
locked/in/*: locked/in/h
l*/in/*: locked/in/h
l*/*/h: l*/*/h
d?/sub/: d1/sub/ d2/sub/
"d1"/*/f: d1/sub/f
d\1/*/f: d1/sub/f

== stderr 0

== merged 197
*/sub: d1/sub d2/sub d3/sub
*/sub/*: d1/sub/f d2/sub/g
locked/*: locked/*
locked/in/*: locked/in/h
l*/in/*: locked/in/h
l*/*/h: l*/*/h
d?/sub/: d1/sub/ d2/sub/
"d1"/*/f: d1/sub/f
d\1/*/f: d1/sub/f

== status 0
