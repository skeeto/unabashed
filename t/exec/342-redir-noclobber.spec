== stdout 104
status 1
[first]
[third]
[third]
[fourth]
[new]
status 0
status 1
status 1
[fifth]
status 1
rw
status 0

== stderr 215
bash: line 5: /tmp/f: cannot overwrite existing file
bash: line 16: /tmp/f: cannot overwrite existing file
bash: line 19: /tmp/g: cannot overwrite existing file
bash: line 25: /tmp/f: cannot overwrite existing file

== merged 319
bash: line 5: /tmp/f: cannot overwrite existing file
status 1
[first]
[third]
[third]
[fourth]
[new]
status 0
bash: line 16: /tmp/f: cannot overwrite existing file
status 1
bash: line 19: /tmp/g: cannot overwrite existing file
status 1
[fifth]
bash: line 25: /tmp/f: cannot overwrite existing file
status 1
rw
status 0

== status 0
