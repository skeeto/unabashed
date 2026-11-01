== stdout 158
hex 16 255 3735928559
oct 8 511 0 0
base2 10 0
base8 15 base10 10 base16 255 255
base36 35 35 36
base64 10 36 62 63 64
neg -16 -3
var 32 33
var2 6
varbase 31

== stderr 137
bash: line 6: 37#Z: value too great for base (error token is "37#Z")
bash: line 15: 10#: invalid integer constant (error token is "10#")

== merged 295
hex 16 255 3735928559
oct 8 511 0 0
base2 10 0
base8 15 base10 10 base16 255 255
base36 35 35 36
bash: line 6: 37#Z: value too great for base (error token is "37#Z")
base64 10 36 62 63 64
neg -16 -3
var 32 33
var2 6
varbase 31
bash: line 15: 10#: invalid integer constant (error token is "10#")

== status 1
