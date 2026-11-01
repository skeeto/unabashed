== stdout 809
${!-x} = [x]
${!:-x} = [x]
${!+x} = []
${!?} = [bash]
${!#} = [b]
${!} = []
bash: line 7: [${#:}]: bad substitution
bash: line 7: [${#/}]: bad substitution
${#/a} = [2]
bash: line 7: [${#%}]: bad substitution
${#%a} = [2]
bash: line 7: [${#=}]: bad substitution
bash: line 7: [${#+}]: bad substitution
${#+x} = [x]
${#-x} = [2]
${#?} = [1]
${#?x} = [2]
${#:-x} = [2]
bash: line 7: [${#^}]: bad substitution
${#2} = [1]
${#@} = [2]
${##} = [1]
${###} = [2]
${#%%} = [2]
bash: line 7: $!: cannot assign in this way
bash: line 7: [${#1xyz}]: bad substitution
bash: line 7: [${#@x}]: bad substitution
${#@Q} = ['2']
${!-} = []
bash: line 7: [${!$}]: bad substitution
bash: line 7: [${!#x}]: bad substitution
${!%} = []
bash: line 7: [${!@Q}]: bad substitution
${!:0} = []
bash: line 7: a b: invalid variable name

== stderr 0

== merged 809
${!-x} = [x]
${!:-x} = [x]
${!+x} = []
${!?} = [bash]
${!#} = [b]
${!} = []
bash: line 7: [${#:}]: bad substitution
bash: line 7: [${#/}]: bad substitution
${#/a} = [2]
bash: line 7: [${#%}]: bad substitution
${#%a} = [2]
bash: line 7: [${#=}]: bad substitution
bash: line 7: [${#+}]: bad substitution
${#+x} = [x]
${#-x} = [2]
${#?} = [1]
${#?x} = [2]
${#:-x} = [2]
bash: line 7: [${#^}]: bad substitution
${#2} = [1]
${#@} = [2]
${##} = [1]
${###} = [2]
${#%%} = [2]
bash: line 7: $!: cannot assign in this way
bash: line 7: [${#1xyz}]: bad substitution
bash: line 7: [${#@x}]: bad substitution
${#@Q} = ['2']
${!-} = []
bash: line 7: [${!$}]: bad substitution
bash: line 7: [${!#x}]: bad substitution
${!%} = []
bash: line 7: [${!@Q}]: bad substitution
${!:0} = []
bash: line 7: a b: invalid variable name

== status 1
