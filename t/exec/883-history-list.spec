== stdout 413
one
a
b
here
    1  echo one
    2  for i in a b; do   echo $i; done
    3  f() {   echo in f; }
    4  cat <<END
here
END

    5  history
hidden
dup
dup
/
    6  HISTCONTROL=ignoreboth
    7  echo dup
    8  HISTIGNORE='ls*:&'
    9  history 4
st 1
st 1
st 2
st 2
st 1
st 2
   15  pushed one
   16  pushed two
   17  history 3
echo history 3
st 1
   16  history 3
   17  HISTSIZE=3
   18  history
    1  history

== stderr 646
bash: line 25: history: 99: history position out of range
bash: line 26: history: 1-x: history position out of range
bash: line 27: history: -d: option requires an argument
history: usage: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
bash: line 28: history: -x: invalid option
history: usage: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
bash: line 29: history: cannot use more than one of -anrw
bash: line 30: history: abc: numeric argument required
bash: line 33: history: !pushed:2: history expansion failed
bash: line 33: history: !!:z: history expansion failed

== merged 1059
one
a
b
here
    1  echo one
    2  for i in a b; do   echo $i; done
    3  f() {   echo in f; }
    4  cat <<END
here
END

    5  history
hidden
dup
dup
/
    6  HISTCONTROL=ignoreboth
    7  echo dup
    8  HISTIGNORE='ls*:&'
    9  history 4
bash: line 25: history: 99: history position out of range
st 1
bash: line 26: history: 1-x: history position out of range
st 1
bash: line 27: history: -d: option requires an argument
history: usage: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
st 2
bash: line 28: history: -x: invalid option
history: usage: history [-c] [-d offset] [n] or history -anrw [filename] or history -ps arg [arg...]
st 2
bash: line 29: history: cannot use more than one of -anrw
st 1
bash: line 30: history: abc: numeric argument required
st 2
   15  pushed one
   16  pushed two
   17  history 3
echo history 3
bash: line 33: history: !pushed:2: history expansion failed
bash: line 33: history: !!:z: history expansion failed
st 1
   16  history 3
   17  HISTSIZE=3
   18  history
    1  history

== status 0
