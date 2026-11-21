== stdout 70
printf 1
third 1
each echo wrote its line or failed: 1
some failed: 1

== stderr 101
bash: line 8: printf: write error: File too large
bash: line 10: printf: write error: File too large

== merged 171
bash: line 8: printf: write error: File too large
printf 1
bash: line 10: printf: write error: File too large
third 1
each echo wrote its line or failed: 1
some failed: 1

== status 0
