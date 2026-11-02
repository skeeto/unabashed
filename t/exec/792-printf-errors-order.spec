== stdout 666
a
bash: line 4: printf: x: invalid number
0b
a
bash: line 5: printf: `\': invalid format character
a
bash: line 6: printf: `1bad': not a valid identifier
xa
x
a
bash: line 8: printf: warning: `Z': invalid time format specification
bash: line 8: printf: `Y': invalid format character
%(a
bash: line 9: printf: 99999999999: Numerical result out of range
x
a
bash: line 10: printf: `%5': missing format character

a
bash: line 12: printf: missing hex digit for \x
\x
a
bash: line 13: printf: missing hex digit for \x
\x
a
x
bash: line 14: printf: y: invalid number
0
bash: line 15: printf: x: invalid number
a
0
one
two
three
bash: line 16: printf: z: invalid number
0

== stderr 0

== merged 666
a
bash: line 4: printf: x: invalid number
0b
a
bash: line 5: printf: `\': invalid format character
a
bash: line 6: printf: `1bad': not a valid identifier
xa
x
a
bash: line 8: printf: warning: `Z': invalid time format specification
bash: line 8: printf: `Y': invalid format character
%(a
bash: line 9: printf: 99999999999: Numerical result out of range
x
a
bash: line 10: printf: `%5': missing format character

a
bash: line 12: printf: missing hex digit for \x
\x
a
bash: line 13: printf: missing hex digit for \x
\x
a
x
bash: line 14: printf: y: invalid number
0
bash: line 15: printf: x: invalid number
a
0
one
two
three
bash: line 16: printf: z: invalid number
0

== status 1
