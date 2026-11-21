== stdout 41
1 2
2 2
3 2
4 2
5 0
6 0
7 1
8 0
9 0
10 2

== stderr 438
bash: line 5: [[: invalid regular expression `^[à-ÿ]$': Invalid collation character
bash: line 6: [[: invalid regular expression `^[а-я]$': Invalid collation character
bash: line 7: [[: invalid regular expression `^[a-é]$': Invalid collation character
bash: line 8: [[: invalid regular expression `^[[.é.]-ÿ]$': Invalid collation character
bash: line 15: [[: invalid regular expression `^[[.é.]-ÿ]$': Invalid collation character

== merged 479
bash: line 5: [[: invalid regular expression `^[à-ÿ]$': Invalid collation character
1 2
bash: line 6: [[: invalid regular expression `^[а-я]$': Invalid collation character
2 2
bash: line 7: [[: invalid regular expression `^[a-é]$': Invalid collation character
3 2
bash: line 8: [[: invalid regular expression `^[[.é.]-ÿ]$': Invalid collation character
4 2
5 0
6 0
7 1
8 0
9 0
bash: line 15: [[: invalid regular expression `^[[.é.]-ÿ]$': Invalid collation character
10 2

== status 0
