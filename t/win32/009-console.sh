# conrun: await:ready 'type:plain\n' 'type:caf\u00e9 \u2713\n' sleep:2500 'type:late\n' 'type:\z\n'
# Console input: UTF-8 whatever the code page, a line at a time; a
# timed-out read loses nothing; Ctrl-Z at a line's start ends input
: >ready
read a; echo "a=$a"
read b; echo "b=$b"
read -t 1 c; echo "st=$? c=[$c]"
read d; echo "d=$d"
read e; echo "eof $?"
