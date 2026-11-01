== stdout 72
got usr1
wait 138
late
killed 143
w 143
1 0 0
2 0 0
3 141 0
4 0 0
5 0 0

== stderr 167
bash: line 15: echo: write error: Broken pipe
caught
st 1
bash: line 17: echo: write error: Broken pipe
st 1
bash: line 22: echo: write error: Broken pipe
st 1
st 141

== merged 239
got usr1
wait 138
late
killed 143
w 143
bash: line 15: echo: write error: Broken pipe
caught
st 1
1 0 0
bash: line 17: echo: write error: Broken pipe
st 1
2 0 0
3 141 0
bash: line 22: echo: write error: Broken pipe
st 1
4 0 0
st 141
5 0 0

== status 0
