== stdout 164
declare -a arr=([0]="line one of the here-document, long enough to share a class")
declare -a arr3=([0]="a" [1]="b" [2]="d" [3]="e")
declare -- skipped=""
status 3

== stderr 0

== merged 164
declare -a arr=([0]="line one of the here-document, long enough to share a class")
declare -a arr3=([0]="a" [1]="b" [2]="d" [3]="e")
declare -- skipped=""
status 3

== status 0
