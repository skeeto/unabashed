== stdout 186
[in]
back
f [a
bash: line 11: 1/0: division by 0 (error token is "0")] 1
to stderr
[]
[bg
bg2
fg]
[bash: line 21: 1/0: division by 0 (error token is "0")]
[in] [new]
[in] [new]
trap out

== stderr 0

== merged 186
[in]
back
f [a
bash: line 11: 1/0: division by 0 (error token is "0")] 1
to stderr
[]
[bg
bg2
fg]
[bash: line 21: 1/0: division by 0 (error token is "0")]
[in] [new]
[in] [new]
trap out

== status 127
