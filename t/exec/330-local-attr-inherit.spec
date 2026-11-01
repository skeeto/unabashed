== stdout 175
local gi=3+4
declare -- gi="3+4"
global gi=7
local gu=abc
redecl gu=abc
declare -x gx="2"
declare -i gi="7"
declare -u gu="X"
declare -x gx="1"
inner cr=2 status 0
outer cr=1

== stderr 0

== merged 175
local gi=3+4
declare -- gi="3+4"
global gi=7
local gu=abc
redecl gu=abc
declare -x gx="2"
declare -i gi="7"
declare -u gu="X"
declare -x gx="1"
inner cr=2 status 0
outer cr=1

== status 0
