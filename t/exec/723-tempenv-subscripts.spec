== stdout 129
bash: line 2: declare: var: not found
f: 1
st=0
st=0
bash: line 4: declare: var: not found
st=1
declare -x var="(1 2)"
f: 0
done

== stderr 186
bash: line 3: `var[0]': not a valid identifier
bash: line 3: `var[@]': not a valid identifier
bash: line 4: `var[0]': not a valid identifier
bash: line 5: `v[1]': not a valid identifier

== merged 315
bash: line 3: `var[0]': not a valid identifier
bash: line 3: `var[@]': not a valid identifier
bash: line 2: declare: var: not found
f: 1
st=0
bash: line 4: `var[0]': not a valid identifier
st=0
bash: line 4: declare: var: not found
bash: line 5: `v[1]': not a valid identifier
st=1
declare -x var="(1 2)"
f: 0
done

== status 0
