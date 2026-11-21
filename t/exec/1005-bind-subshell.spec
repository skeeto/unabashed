== stdout 425
sub: set bell-style none "\C-t": "hello"
changed: set bell-style visible
"\C-t": "bye"
set bell-style none
"\C-t": "hello"
set bell-style audible
( ) 0
set bell-style none
"\C-t": "hello"
"\C-t": "piped"
"\C-t": "hello"
"\C-t": "hello"
"\C-t": "hello"
"\C-t": "inner"
"\C-t": "outer"
"\C-t": "hello"
vi: set editing-mode emacs
set editing-mode vi
emacs          	on
vi             	off
emacs          	off
vi             	on

== stderr 0

== merged 425
sub: set bell-style none "\C-t": "hello"
changed: set bell-style visible
"\C-t": "bye"
set bell-style none
"\C-t": "hello"
set bell-style audible
( ) 0
set bell-style none
"\C-t": "hello"
"\C-t": "piped"
"\C-t": "hello"
"\C-t": "hello"
"\C-t": "hello"
"\C-t": "inner"
"\C-t": "outer"
"\C-t": "hello"
vi: set editing-mode emacs
set editing-mode vi
emacs          	on
vi             	off
emacs          	off
vi             	on

== status 0
