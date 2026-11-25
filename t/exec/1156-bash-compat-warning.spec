== stdout 203
[3.1]
[31]
[5.3]
[53]
[5.0]
[]
[3.0]
[30]
[54]
[5.4]
[4]
[4.2.1]
[042]
[a1]
[.4]
[ 42]
builtin
function
program
twice
restored
expanded
export
declare -x BASH_COMPAT="w"
others
local
child
child q
quiet

== stderr 1590
bash: line 6: BASH_COMPAT: 3.0: compatibility value out of range
bash: line 6: BASH_COMPAT: 30: compatibility value out of range
bash: line 6: BASH_COMPAT: 54: compatibility value out of range
bash: line 6: BASH_COMPAT: 5.4: compatibility value out of range
bash: line 6: BASH_COMPAT: 4: compatibility value out of range
bash: line 6: BASH_COMPAT: 4.2.1: compatibility value out of range
bash: line 6: BASH_COMPAT: 042: compatibility value out of range
bash: line 6: BASH_COMPAT: a1: compatibility value out of range
bash: line 6: BASH_COMPAT: .4: compatibility value out of range
bash: line 6: BASH_COMPAT:  42: compatibility value out of range
bash: line 9: BASH_COMPAT: x: compatibility value out of range
bash: line 10: BASH_COMPAT: y: compatibility value out of range
bash: line 12: BASH_COMPAT: pv: compatibility value out of range
bash: line 13: BASH_COMPAT: x: compatibility value out of range
bash: line 13: BASH_COMPAT: pv: compatibility value out of range
bash: line 14: BASH_COMPAT: pv: compatibility value out of range
bash: line 17: BASH_COMPAT: w: compatibility value out of range
bash: line 17: BASH_COMPAT: w: compatibility value out of range
bash: line 17: BASH_COMPAT: w: compatibility value out of range
bash: line 18: BASH_COMPAT: wr: compatibility value out of range
bash: line 18: BASH_COMPAT: rd: compatibility value out of range
bash: line 18: BASH_COMPAT: pv: compatibility value out of range
bash: line 19: BASH_COMPAT: u: compatibility value out of range
bash: BASH_COMPAT: v: compatibility value out of range
q: BASH_COMPAT: v: compatibility value out of range

== merged 1793
[3.1]
[31]
[5.3]
[53]
[5.0]
[]
[3.0]
bash: line 6: BASH_COMPAT: 3.0: compatibility value out of range
[30]
bash: line 6: BASH_COMPAT: 30: compatibility value out of range
[54]
bash: line 6: BASH_COMPAT: 54: compatibility value out of range
[5.4]
bash: line 6: BASH_COMPAT: 5.4: compatibility value out of range
[4]
bash: line 6: BASH_COMPAT: 4: compatibility value out of range
[4.2.1]
bash: line 6: BASH_COMPAT: 4.2.1: compatibility value out of range
[042]
bash: line 6: BASH_COMPAT: 042: compatibility value out of range
[a1]
bash: line 6: BASH_COMPAT: a1: compatibility value out of range
[.4]
bash: line 6: BASH_COMPAT: .4: compatibility value out of range
[ 42]
bash: line 6: BASH_COMPAT:  42: compatibility value out of range
builtin
bash: line 9: BASH_COMPAT: x: compatibility value out of range
function
bash: line 10: BASH_COMPAT: y: compatibility value out of range
program
bash: line 12: BASH_COMPAT: pv: compatibility value out of range
twice
bash: line 13: BASH_COMPAT: x: compatibility value out of range
bash: line 13: BASH_COMPAT: pv: compatibility value out of range
restored
bash: line 14: BASH_COMPAT: pv: compatibility value out of range
expanded
export
bash: line 17: BASH_COMPAT: w: compatibility value out of range
bash: line 17: BASH_COMPAT: w: compatibility value out of range
bash: line 17: BASH_COMPAT: w: compatibility value out of range
declare -x BASH_COMPAT="w"
others
bash: line 18: BASH_COMPAT: wr: compatibility value out of range
bash: line 18: BASH_COMPAT: rd: compatibility value out of range
bash: line 18: BASH_COMPAT: pv: compatibility value out of range
bash: line 19: BASH_COMPAT: u: compatibility value out of range
local
bash: BASH_COMPAT: v: compatibility value out of range
child
q: BASH_COMPAT: v: compatibility value out of range
child q
quiet

== status 0
