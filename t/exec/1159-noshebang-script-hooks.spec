== stdout 60
in f2 x
in f2 x
in f2 x
in f2 4.0
in f2 y
after 4.0
in f2 y

== stderr 498
bash: line 7: BASH_COMPAT: x: compatibility value out of range
bash: BASH_COMPAT: x: compatibility value out of range
bash: BASH_COMPAT: x: compatibility value out of range
bash: BASH_COMPAT: x: compatibility value out of range
zz: BASH_COMPAT: x: compatibility value out of range
bash: BASH_COMPAT: y: compatibility value out of range
./f2: BASH_COMPAT: y: compatibility value out of range
q: BASH_COMPAT: y: compatibility value out of range
./f2: BASH_COMPAT: y: compatibility value out of range

== merged 558
bash: line 7: BASH_COMPAT: x: compatibility value out of range
bash: BASH_COMPAT: x: compatibility value out of range
in f2 x
bash: BASH_COMPAT: x: compatibility value out of range
in f2 x
bash: BASH_COMPAT: x: compatibility value out of range
zz: BASH_COMPAT: x: compatibility value out of range
in f2 x
in f2 4.0
bash: BASH_COMPAT: y: compatibility value out of range
./f2: BASH_COMPAT: y: compatibility value out of range
in f2 y
after 4.0
q: BASH_COMPAT: y: compatibility value out of range
./f2: BASH_COMPAT: y: compatibility value out of range
in f2 y

== status 0
