== stdout 301
declare -- a="plain"
declare -- b="with space"
declare -- c="dq\"inside"
declare -- d="dollar \$HOME and \`tick\`"
declare -- e="back\\slash"
declare -- f=$'new\nline'
declare -- g=$'tab\ttab'
declare -- h="single'quote"
declare -- i=""
declare -- j="!bang"
declare -- k="~tilde"
declare -x x1="a\"b"

== stderr 0

== merged 301
declare -- a="plain"
declare -- b="with space"
declare -- c="dq\"inside"
declare -- d="dollar \$HOME and \`tick\`"
declare -- e="back\\slash"
declare -- f=$'new\nline'
declare -- g=$'tab\ttab'
declare -- h="single'quote"
declare -- i=""
declare -- j="!bang"
declare -- k="~tilde"
declare -x x1="a\"b"

== status 0
