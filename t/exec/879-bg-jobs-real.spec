== stdout 269
bash: Killed                     f
after wait 0
bash: Killed                     sleep 5
status 137
first 3 a
second 4 b
waited -f 5
[1]+  Running                    { sleep 1; } &
jobs gone
DEBUG true
DEBUG wait
DEBUG true
DEBUG true
DEBUG wait
DEBUG trap - DEBUG
end

== stderr 0

== merged 269
bash: Killed                     f
after wait 0
bash: Killed                     sleep 5
status 137
first 3 a
second 4 b
waited -f 5
[1]+  Running                    { sleep 1; } &
jobs gone
DEBUG true
DEBUG wait
DEBUG true
DEBUG true
DEBUG wait
DEBUG trap - DEBUG
end

== status 0
