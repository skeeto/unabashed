== stdout 2153
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
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
0status 1
a\
0status 1
a\
0status 1
a\
0status 1
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 1
a\
status 127
a\
status 127
a\
status 1
a\
status 1
a\
status 127
a\
status 127
a\
status 1
a\
status 1
a\
status 2
a\
status 2
a\
status 2
a\
status 2
a\
status 1
a\
status 127
a\
status 1
a\
status 127

== stderr 4543
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
/tmp/t.sh: line -1: cd: /nonexist: No such file or directory
bash: line -1: cd: /nonexist: No such file or directory
/tmp/t.sh: line 0: cd: /nonexist: No such file or directory
bash: line 0: cd: /nonexist: No such file or directory
/tmp/t.sh: line -1: local: can only be used in a function
bash: line -1: local: can only be used in a function
/tmp/t.sh: line 0: local: can only be used in a function
bash: line 0: local: can only be used in a function
/tmp/t.sh: line -1: printf: x: invalid number
bash: line -1: printf: x: invalid number
/tmp/t.sh: line 0: printf: x: invalid number
bash: line 0: printf: x: invalid number
/tmp/t.sh: line -1: shift: x: numeric argument required
bash: line -1: shift: x: numeric argument required
/tmp/t.sh: line 0: shift: x: numeric argument required
bash: line 0: shift: x: numeric argument required
/tmp/t.sh: line 2: unset: q: cannot unset: readonly variable
bash: line 2: unset: q: cannot unset: readonly variable
/tmp/t.sh: line 0: unset: q: cannot unset: readonly variable
bash: line 0: unset: q: cannot unset: readonly variable
/tmp/t.sh: /nonexist: No such file or directory
bash: /nonexist: No such file or directory
/tmp/t.sh: /nonexist: No such file or directory
bash: /nonexist: No such file or directory
/tmp/t.sh: let: 1/0: division by 0 (error token is "0")
bash: let: 1/0: division by 0 (error token is "0")
/tmp/t.sh: let: 1/0: division by 0 (error token is "0")
bash: let: 1/0: division by 0 (error token is "0")
/tmp/t.sh: line 2: 1/0: No such file or directory
bash: line 2: 1/0: No such file or directory
/tmp/t.sh: ((: 1/0: division by 0 (error token is "0")
bash: ((: 1/0: division by 0 (error token is "0")
/tmp/t.sh: [[: command not found
bash: [[: command not found
/tmp/t.sh: [[: 1/0: division by 0 (error token is "0")
bash: [[: 1/0: division by 0 (error token is "0")
/tmp/t.sh: line 2: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 2: [[: invalid regular expression `(': Unmatched ( or \(
/tmp/t.sh: line 0: [[: invalid regular expression `(': Unmatched ( or \(
bash: line 0: [[: invalid regular expression `(': Unmatched ( or \(
/tmp/t.sh: u: unset
bash: u: unset
/tmp/t.sh: u: unset
bash: u: unset

== merged 6696
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
a\
/tmp/t.sh: line -1: cd: /nonexist: No such file or directory
status 1
a\
bash: line -1: cd: /nonexist: No such file or directory
status 1
a\
/tmp/t.sh: line 0: cd: /nonexist: No such file or directory
status 1
a\
bash: line 0: cd: /nonexist: No such file or directory
status 1
a\
/tmp/t.sh: line -1: local: can only be used in a function
status 1
a\
bash: line -1: local: can only be used in a function
status 1
a\
/tmp/t.sh: line 0: local: can only be used in a function
status 1
a\
bash: line 0: local: can only be used in a function
status 1
a\
/tmp/t.sh: line -1: printf: x: invalid number
0status 1
a\
bash: line -1: printf: x: invalid number
0status 1
a\
/tmp/t.sh: line 0: printf: x: invalid number
0status 1
a\
bash: line 0: printf: x: invalid number
0status 1
a\
/tmp/t.sh: line -1: shift: x: numeric argument required
status 2
a\
bash: line -1: shift: x: numeric argument required
status 2
a\
/tmp/t.sh: line 0: shift: x: numeric argument required
status 2
a\
bash: line 0: shift: x: numeric argument required
status 2
a\
/tmp/t.sh: line 2: unset: q: cannot unset: readonly variable
status 1
a\
bash: line 2: unset: q: cannot unset: readonly variable
status 1
a\
/tmp/t.sh: line 0: unset: q: cannot unset: readonly variable
status 1
a\
bash: line 0: unset: q: cannot unset: readonly variable
status 1
a\
/tmp/t.sh: /nonexist: No such file or directory
status 1
a\
bash: /nonexist: No such file or directory
status 1
a\
/tmp/t.sh: /nonexist: No such file or directory
status 1
a\
bash: /nonexist: No such file or directory
status 1
a\
/tmp/t.sh: let: 1/0: division by 0 (error token is "0")
status 1
a\
bash: let: 1/0: division by 0 (error token is "0")
status 1
a\
/tmp/t.sh: let: 1/0: division by 0 (error token is "0")
status 1
a\
bash: let: 1/0: division by 0 (error token is "0")
status 1
a\
/tmp/t.sh: line 2: 1/0: No such file or directory
status 127
a\
bash: line 2: 1/0: No such file or directory
status 127
a\
/tmp/t.sh: ((: 1/0: division by 0 (error token is "0")
status 1
a\
bash: ((: 1/0: division by 0 (error token is "0")
status 1
a\
/tmp/t.sh: [[: command not found
status 127
a\
bash: [[: command not found
status 127
a\
/tmp/t.sh: [[: 1/0: division by 0 (error token is "0")
status 1
a\
bash: [[: 1/0: division by 0 (error token is "0")
status 1
a\
/tmp/t.sh: line 2: [[: invalid regular expression `(': Unmatched ( or \(
status 2
a\
bash: line 2: [[: invalid regular expression `(': Unmatched ( or \(
status 2
a\
/tmp/t.sh: line 0: [[: invalid regular expression `(': Unmatched ( or \(
status 2
a\
bash: line 0: [[: invalid regular expression `(': Unmatched ( or \(
status 2
a\
/tmp/t.sh: u: unset
status 1
a\
bash: u: unset
status 127
a\
/tmp/t.sh: u: unset
status 1
a\
bash: u: unset
status 127

== status 0
