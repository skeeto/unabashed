== stdout 49
in exit 127
in exit 127
st 0
exit
err 1
st 0
end

== stderr 176
bash: line 1: nosuchcmd1: command not found
bash: line 1: nosuchcmd1: command not found
bash: line 1: nosuchcmd2: command not found
bash: line 1: nosuchcmd2: command not found

== merged 225
bash: line 1: nosuchcmd1: command not found
bash: line 1: nosuchcmd1: command not found
in exit 127
in exit 127
st 0
exit
err 1
st 0
end
bash: line 1: nosuchcmd2: command not found
bash: line 1: nosuchcmd2: command not found

== status 0
