== stdout 340
T: [[ 1 -eq 1 ]]
T: [[ 1+1 -eq 2 ]]
T: [[ "2 * 3" -eq 6 ]]
T: [[ x -eq 3 ]]
T: [[ $x -eq 3 ]]
T: [[ x+1 -gt x ]]
T: [[ 0x10 -eq 16 ]]
T: [[ 010 -eq 8 ]]
T: [[ 2#101 -eq 5 ]]
T: [[ -1 -lt 0 ]]
T: [[ 5 -le 5 ]]
F: [[ 5 -ne 5 ]]
F: [[ 7 -ge 8 ]]
T: [[ unsetvar -eq 0 ]]
T: [[ "" -eq 0 ]]
T: [[ " 4 " -eq 4 ]]
T: [[ e -eq 6 ]]
i=1
i=2 status=0

== stderr 177
bash: -c: line 24: syntax error in conditional expression: unexpected token `-eq'
bash: -c: line 24: syntax error near `-eq'
bash: -c: line 24: `[[ (j=5) -eq 5 ]]; echo "j=$j"'

== merged 517
T: [[ 1 -eq 1 ]]
T: [[ 1+1 -eq 2 ]]
T: [[ "2 * 3" -eq 6 ]]
T: [[ x -eq 3 ]]
T: [[ $x -eq 3 ]]
T: [[ x+1 -gt x ]]
T: [[ 0x10 -eq 16 ]]
T: [[ 010 -eq 8 ]]
T: [[ 2#101 -eq 5 ]]
T: [[ -1 -lt 0 ]]
T: [[ 5 -le 5 ]]
F: [[ 5 -ne 5 ]]
F: [[ 7 -ge 8 ]]
T: [[ unsetvar -eq 0 ]]
T: [[ "" -eq 0 ]]
T: [[ " 4 " -eq 4 ]]
T: [[ e -eq 6 ]]
i=1
i=2 status=0
bash: -c: line 24: syntax error in conditional expression: unexpected token `-eq'
bash: -c: line 24: syntax error near `-eq'
bash: -c: line 24: `[[ (j=5) -eq 5 ]]; echo "j=$j"'

== status 2
