== stdout 218
*/: d1/ d2/ l1/
*//: d1/ d2/ l1/ /
*//: d1/ d2/ l1/
.*/: .d3/
d*/.: d1/. d2/.
*/..: d1/.. d2/.. l1/..
/tmp/ts/d*/: /tmp/ts/d1/ /tmp/ts/d2/
//tmp//ts//d?: //tmp//ts//d1 //tmp//ts//d2
./*1: ./d1 ./f1 ./l1
.//l*/: .//l1/

== stderr 0

== merged 218
*/: d1/ d2/ l1/
*//: d1/ d2/ l1/ /
*//: d1/ d2/ l1/
.*/: .d3/
d*/.: d1/. d2/.
*/..: d1/.. d2/.. l1/..
/tmp/ts/d*/: /tmp/ts/d1/ /tmp/ts/d2/
//tmp//ts//d?: //tmp//ts//d1 //tmp//ts//d2
./*1: ./d1 ./f1 ./l1
.//l*/: .//l1/

== status 0
