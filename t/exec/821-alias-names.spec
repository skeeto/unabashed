== stdout 457
bash: line 4: alias: `\$': invalid alias name
1
bash: line 5: alias: `a b': invalid alias name
bash: line 5: alias: `a/b': invalid alias name
bash: line 5: alias: `a$': invalid alias name
bash: line 5: alias: `a;': invalid alias name
bash: line 5: alias: `a"': invalid alias name
bash: line 5: alias: `a<': invalid alias name
1
bash: line 6: `\$': invalid alias name
0
0
alias a=b='x'
alias ok-name='x'
bar
foo
bash: line 15: type: foo: not found
foo bar x

== stderr 0

== merged 457
bash: line 4: alias: `\$': invalid alias name
1
bash: line 5: alias: `a b': invalid alias name
bash: line 5: alias: `a/b': invalid alias name
bash: line 5: alias: `a$': invalid alias name
bash: line 5: alias: `a;': invalid alias name
bash: line 5: alias: `a"': invalid alias name
bash: line 5: alias: `a<': invalid alias name
1
bash: line 6: `\$': invalid alias name
0
0
alias a=b='x'
alias ok-name='x'
bar
foo
bash: line 15: type: foo: not found
foo bar x

== status 0
