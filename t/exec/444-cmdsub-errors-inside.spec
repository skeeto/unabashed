== stdout 37
st=127 x=[]
y=[]
st=1 z=[]
st=1
st=1

== stderr 198
bash: line 2: nosuchcmd_xyz: command not found
bash: line 4: r: readonly variable
bash: line 6: unset_q: is unset
bash: line 8: unset_q2: gone
bash: line 10: cd: /nowhere: No such file or directory

== merged 235
bash: line 2: nosuchcmd_xyz: command not found
st=127 x=[]
bash: line 4: r: readonly variable
y=[]
bash: line 6: unset_q: is unset
st=1 z=[]
bash: line 8: unset_q2: gone
st=1
bash: line 10: cd: /nowhere: No such file or directory
st=1

== status 0
