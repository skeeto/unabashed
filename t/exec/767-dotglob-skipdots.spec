== stdout 452
globskipdots        	on
dotglob             	off
*: c dd
.*: .a .bb .h
.?: .a .h
.??: .bb
[.]*: [.]*
.h/*: .h/j
.h/.*: .h/.i
*/*: */*
noskip .*: . .. .a .bb .h
noskip .?: .. .a .h
noskip *: c dd
noskip .h/.*: .h/. .h/.. .h/.i
dot noskip *: .a .bb .h c dd
dot noskip .*: . .. .a .bb .h
dot noskip ?: c
dot *: .a .bb .h c dd
dot .*: .a .bb .h
dot */*: .h/.i .h/j
dot ??: .a .h dd
dot **: .a .bb .h .h/.i .h/j c dd
**: c dd
.h/**: .h/ .h/j
.*/**: .h .h/j

== stderr 0

== merged 452
globskipdots        	on
dotglob             	off
*: c dd
.*: .a .bb .h
.?: .a .h
.??: .bb
[.]*: [.]*
.h/*: .h/j
.h/.*: .h/.i
*/*: */*
noskip .*: . .. .a .bb .h
noskip .?: .. .a .h
noskip *: c dd
noskip .h/.*: .h/. .h/.. .h/.i
dot noskip *: .a .bb .h c dd
dot noskip .*: . .. .a .bb .h
dot noskip ?: c
dot *: .a .bb .h c dd
dot .*: .a .bb .h
dot */*: .h/.i .h/j
dot ??: .a .h dd
dot **: .a .bb .h .h/.i .h/j c dd
**: c dd
.h/**: .h/ .h/j
.*/**: .h .h/j

== status 0
