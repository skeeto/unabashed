== stdout 218
F: test v -eq 7 (2)
T: [[ v -eq 7 ]]
F: test 0x7 -eq 7 (2)
T: [[ 0x7 -eq 7 ]]
T: test 08 -eq 8
T: [[ 10#08 -eq 8 ]]
T: test " -3 " -lt 0
T: [[ " -3 " -lt 0 ]]
T: [[ $v -eq v ]]
F: [[ v == 7 ]] (1)
T: [[ "v" -eq "7" ]]

== stderr 82
bash: line 1: test: v: integer expected
bash: line 1: test: 0x7: integer expected

== merged 300
bash: line 1: test: v: integer expected
F: test v -eq 7 (2)
T: [[ v -eq 7 ]]
bash: line 1: test: 0x7: integer expected
F: test 0x7 -eq 7 (2)
T: [[ 0x7 -eq 7 ]]
T: test 08 -eq 8
T: [[ 10#08 -eq 8 ]]
T: test " -3 " -lt 0
T: [[ " -3 " -lt 0 ]]
T: [[ $v -eq v ]]
F: [[ v == 7 ]] (1)
T: [[ "v" -eq "7" ]]

== status 0
