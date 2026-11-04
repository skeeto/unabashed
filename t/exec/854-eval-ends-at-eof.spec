== stdout 1609
a\
a\
-1
18
a\
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
127
status 0
a\
127
status 0
a\
status 127
a\
status 127
a\
status 127
a\
status 127
a\
status 127
a\
status 127
a\
status 127
a\
status 127
a\
-1
status 0
a\
-1
status 0
a\
a  b 2
status 0
a\
a  b 2
status 0
a\
r 2
r 2
status 0
a\
r 2
r 2
status 0
a\
paren 2
status 0
a\
paren 2
status 0
a\
cs -1 bq 2
status 0
a\
cs -1 bq 2
status 0
a\
fn 2
status 0
a\
fn 2
status 0
a\
blank 3
status 0
a\
blank 3
status 0
a\
comment 3
status 0
a\
comment 3
status 0
a
comment2 -1
status 0
a
comment2 -1
status 0
a
contnl -1
status 0
a
contnl -1
status 0
a\\
three -1
status 0
a\\
three -1
status 0
a\
even 2
status 0
a\
even 2
status 0
a\
-1
0
status 0
a\
-1
0
status 0
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
empty 2
status 0
a\
empty 2
status 0
a\
status 127
a\
status 127
a\
inner -1
if
status 0
a\
inner -1
if
status 0
a\
outer -1
status 0
a\
outer -1
status 0
a\
semi 2
func -1
status 0
a\
semi 2
func -1
status 0
a\
a\
loop -1
status 0
a\
a\
loop -1
status 0
a\
inbrace 1
after -1
status 0
a\
inbrace 1
after -1
status 0
a\
command -1
status 0
a\
command -1
status 0
a\
sub 2
status 0
a\
sub 2
status 0
a\
cs2 2
status 0
a\
cs2 2
status 0
a\
pipe 2
status 0
a\
pipe 2
status 0
src\
src
status 0
src\
src
status 0
a\
src2 3
status 0
a\
src2 3
status 0
t\
trap 3
status 0
t\
trap 3
status 0
a\
t\
trap2 -1
status 0
a\
t\
trap2 -1
status 0
d\
debug 2
status 0
d\
debug 2
status 0
a\
funsub -1
status 0
a\
funsub -1
status 0
a\b
funsub2 2
status 0
a\b
funsub2 2
status 0
a\
status 2
a\
status 2

== stderr 2359
bash: eval: line 7: unexpected EOF while looking for matching `"'
bash: eval: line 8: unexpected EOF while looking for matching `"'
bash: eval: line 10: unexpected EOF while looking for matching `"'
bash: eval: line 11: unexpected EOF while looking for matching `"'
bash: eval: line 12: unexpected EOF while looking for matching `''
bash: eval: line 13: unexpected EOF while looking for matching `}'
bash: eval: line -1: unexpected EOF while looking for matching `"'
bash: eval: line 16: unexpected EOF while looking for matching `"'
bash: eval: line 0: unexpected EOF while looking for matching `"'
/tmp/t.sh: line 2: syntax error near unexpected token `then'
/tmp/t.sh: line 2: `if :; then echo if; fi'
bash: -c: line 2: syntax error near unexpected token `then'
bash: -c: line 2: `if :; then echo if; fi'
/tmp/t.sh: line 2: syntax error near unexpected token `}'
/tmp/t.sh: line 2: `{ echo brace; }'
bash: -c: line 2: syntax error near unexpected token `}'
bash: -c: line 2: `{ echo brace; }'
/tmp/t.sh: line 2: syntax error near unexpected token `)'
/tmp/t.sh: line 2: `case x in x) echo case;; esac'
bash: -c: line 2: syntax error near unexpected token `)'
bash: -c: line 2: `case x in x) echo case;; esac'
/tmp/t.sh: line 2: syntax error near unexpected token `1'
/tmp/t.sh: line 2: `x=(1 2)'
bash: -c: line 2: syntax error near unexpected token `1'
bash: -c: line 2: `x=(1 2)'
/tmp/t.sh: !: command not found
bash: !: command not found
/tmp/t.sh: [[: command not found
bash: [[: command not found
/tmp/t.sh: line 2: 1: command not found
bash: line 2: 1: command not found
/tmp/t.sh: time: command not found
bash: time: command not found
/tmp/t.sh: nosuch: command not found
bash: nosuch: command not found
/tmp/t.sh: eval: line 0: syntax error: unexpected end of file from `if' command on line -1
bash: eval: line 0: syntax error: unexpected end of file from `if' command on line -1
/tmp/t.sh: eval: line -1: syntax error near unexpected token `('
/tmp/t.sh: eval: line -1: `echo (('
bash: eval: line -1: syntax error near unexpected token `('
bash: eval: line -1: `echo (('
/tmp/t.sh: e: command not found
bash: e: command not found
/tmp/t.sh: line 2: syntax error near unexpected token `then'
/tmp/t.sh: line 2: `if :; then echo mapfile; fi'
bash: -c: line 2: syntax error near unexpected token `then'
bash: -c: line 2: `if :; then echo mapfile; fi'

== merged 3968
bash: eval: line 7: unexpected EOF while looking for matching `"'
bash: eval: line 8: unexpected EOF while looking for matching `"'
bash: eval: line 10: unexpected EOF while looking for matching `"'
bash: eval: line 11: unexpected EOF while looking for matching `"'
bash: eval: line 12: unexpected EOF while looking for matching `''
bash: eval: line 13: unexpected EOF while looking for matching `}'
a\
bash: eval: line -1: unexpected EOF while looking for matching `"'
bash: eval: line 16: unexpected EOF while looking for matching `"'
a\
-1
18
a\
bash: eval: line 0: unexpected EOF while looking for matching `"'
a\
/tmp/t.sh: line 2: syntax error near unexpected token `then'
/tmp/t.sh: line 2: `if :; then echo if; fi'
status 2
a\
bash: -c: line 2: syntax error near unexpected token `then'
bash: -c: line 2: `if :; then echo if; fi'
status 2
a\
/tmp/t.sh: line 2: syntax error near unexpected token `}'
/tmp/t.sh: line 2: `{ echo brace; }'
status 2
a\
bash: -c: line 2: syntax error near unexpected token `}'
bash: -c: line 2: `{ echo brace; }'
status 2
a\
/tmp/t.sh: line 2: syntax error near unexpected token `)'
/tmp/t.sh: line 2: `case x in x) echo case;; esac'
status 2
a\
bash: -c: line 2: syntax error near unexpected token `)'
bash: -c: line 2: `case x in x) echo case;; esac'
status 2
a\
/tmp/t.sh: line 2: syntax error near unexpected token `1'
/tmp/t.sh: line 2: `x=(1 2)'
status 2
a\
bash: -c: line 2: syntax error near unexpected token `1'
bash: -c: line 2: `x=(1 2)'
status 2
a\
/tmp/t.sh: !: command not found
127
status 0
a\
bash: !: command not found
127
status 0
a\
/tmp/t.sh: [[: command not found
status 127
a\
bash: [[: command not found
status 127
a\
/tmp/t.sh: line 2: 1: command not found
status 127
a\
bash: line 2: 1: command not found
status 127
a\
/tmp/t.sh: time: command not found
status 127
a\
bash: time: command not found
status 127
a\
/tmp/t.sh: nosuch: command not found
status 127
a\
bash: nosuch: command not found
status 127
a\
-1
status 0
a\
-1
status 0
a\
a  b 2
status 0
a\
a  b 2
status 0
a\
r 2
r 2
status 0
a\
r 2
r 2
status 0
a\
paren 2
status 0
a\
paren 2
status 0
a\
cs -1 bq 2
status 0
a\
cs -1 bq 2
status 0
a\
fn 2
status 0
a\
fn 2
status 0
a\
blank 3
status 0
a\
blank 3
status 0
a\
comment 3
status 0
a\
comment 3
status 0
a
comment2 -1
status 0
a
comment2 -1
status 0
a
contnl -1
status 0
a
contnl -1
status 0
a\\
three -1
status 0
a\\
three -1
status 0
a\
even 2
status 0
a\
even 2
status 0
a\
-1
0
status 0
a\
-1
0
status 0
a\
/tmp/t.sh: eval: line 0: syntax error: unexpected end of file from `if' command on line -1
status 2
a\
bash: eval: line 0: syntax error: unexpected end of file from `if' command on line -1
status 2
a\
/tmp/t.sh: eval: line -1: syntax error near unexpected token `('
/tmp/t.sh: eval: line -1: `echo (('
status 2
a\
bash: eval: line -1: syntax error near unexpected token `('
bash: eval: line -1: `echo (('
status 2
a\
empty 2
status 0
a\
empty 2
status 0
a\
/tmp/t.sh: e: command not found
status 127
a\
bash: e: command not found
status 127
a\
inner -1
if
status 0
a\
inner -1
if
status 0
a\
outer -1
status 0
a\
outer -1
status 0
a\
semi 2
func -1
status 0
a\
semi 2
func -1
status 0
a\
a\
loop -1
status 0
a\
a\
loop -1
status 0
a\
inbrace 1
after -1
status 0
a\
inbrace 1
after -1
status 0
a\
command -1
status 0
a\
command -1
status 0
a\
sub 2
status 0
a\
sub 2
status 0
a\
cs2 2
status 0
a\
cs2 2
status 0
a\
pipe 2
status 0
a\
pipe 2
status 0
src\
src
status 0
src\
src
status 0
a\
src2 3
status 0
a\
src2 3
status 0
t\
trap 3
status 0
t\
trap 3
status 0
a\
t\
trap2 -1
status 0
a\
t\
trap2 -1
status 0
d\
debug 2
status 0
d\
debug 2
status 0
a\
funsub -1
status 0
a\
funsub -1
status 0
a\b
funsub2 2
status 0
a\b
funsub2 2
status 0
a\
/tmp/t.sh: line 2: syntax error near unexpected token `then'
/tmp/t.sh: line 2: `if :; then echo mapfile; fi'
status 2
a\
bash: -c: line 2: syntax error near unexpected token `then'
bash: -c: line 2: `if :; then echo mapfile; fi'
status 2

== status 0
