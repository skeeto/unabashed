== stdout 170
[joined here V]
[not \joined]
[double EOF2]
[while IFS= read -r l; do echo "{}"; done <<'EOF2']
[kept here V]
<dollar at end $>
<dollar-space $ x>
<dollar-brace-less Vx>

== stderr 0

== merged 170
[joined here V]
[not \joined]
[double EOF2]
[while IFS= read -r l; do echo "{}"; done <<'EOF2']
[kept here V]
<dollar at end $>
<dollar-space $ x>
<dollar-brace-less Vx>

== status 0
