== stdout 2038
-- cat <<X;;a ${x;;X;;echo after $?
bash: line 1: a ${x
: bad substitution
after 1
status 0
-- cat <<X;;a ${x b;;c;;X;;echo after $?
bash: line 1: a ${x b
c
: bad substitution
after 1
status 0
-- cat <<X;;a ${x:-{};;X;;echo after $?
a {
after 0
status 0
-- x=1; cat <<X;;${x;;y;;X
bash: line 1: ${x
y
: bad substitution
status 1
-- cat <<X;;a ${x;;b };;X
bash: line 1: a ${x
b }
: bad substitution
status 1
-- cat <<X;;${x[;;X
bash: line 1: ${x[
: bad substitution
status 1
-- cat <<-X;;	${x;;	X
bash: line 1: ${x
: bad substitution
status 1
-- cat <<X;;\${x;;X
${x
status 0
-- cat <<X;;${;;X
bash: command substitution: line 3: unexpected EOF while looking for matching `}'
status 1
-- cat <<X;;${#;;X
bash: line 1: ${#
: bad substitution
status 1
-- cat <<X;;a $((1+(2);;X
bash: line 1: bad substitution: no closing `)' in a $((1+(2)

status 1
-- cat <<X;;a $((1)) $((2;;X;;echo $?
bash: line 1: bad substitution: no closing `)' in a $((1)) $((2

1
status 0
-- cat <<X;;${x:-a;;X
bash: line 1: bad substitution: no closing `}' in ${x:-a

status 1
-- cat <<X;;a ${x:;;X
bash: line 1: bad substitution: no closing `}' in a ${x:

status 1
-- cat <<X;;a ${#x:-b;;X
bash: line 1: a ${#x:-b
: bad substitution
status 1
-- cat <<X;;a ${!x:-b;;X
bash: line 1: x: invalid indirect expansion
status 1
-- x=y; cat <<X;;a ${!x:-b;;X
bash: line 1: bad substitution: no closing `}' in a ${!x:-b

status 1
-- cat <<X;;a ${x[1]#b;;X
bash: line 1: bad substitution: no closing `}' in a ${x[1]#b

status 1
-- cat <<X;;a ${1%b;;X
bash: line 1: bad substitution: no closing `}' in a ${1%b

status 1
-- cat <<X;;a ${x b:-c;;X
bash: line 1: a ${x b:-c
: bad substitution
status 1
-- cat <<X;;a ${x/b;;X
bash: line 1: bad substitution: no closing `}' in a ${x/b

status 1
-- cat <<X;;a ${x@;;X
bash: line 1: bad substitution: no closing `}' in a ${x@

status 1
-- a=(1); cat <<X;;a ${a[1/0]:-b;;X
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; cat <<X;;a ${u%b;;X
bash: line 1: bad substitution: no closing `}' in a ${u%b

status 1

== stderr 0

== merged 2038
-- cat <<X;;a ${x;;X;;echo after $?
bash: line 1: a ${x
: bad substitution
after 1
status 0
-- cat <<X;;a ${x b;;c;;X;;echo after $?
bash: line 1: a ${x b
c
: bad substitution
after 1
status 0
-- cat <<X;;a ${x:-{};;X;;echo after $?
a {
after 0
status 0
-- x=1; cat <<X;;${x;;y;;X
bash: line 1: ${x
y
: bad substitution
status 1
-- cat <<X;;a ${x;;b };;X
bash: line 1: a ${x
b }
: bad substitution
status 1
-- cat <<X;;${x[;;X
bash: line 1: ${x[
: bad substitution
status 1
-- cat <<-X;;	${x;;	X
bash: line 1: ${x
: bad substitution
status 1
-- cat <<X;;\${x;;X
${x
status 0
-- cat <<X;;${;;X
bash: command substitution: line 3: unexpected EOF while looking for matching `}'
status 1
-- cat <<X;;${#;;X
bash: line 1: ${#
: bad substitution
status 1
-- cat <<X;;a $((1+(2);;X
bash: line 1: bad substitution: no closing `)' in a $((1+(2)

status 1
-- cat <<X;;a $((1)) $((2;;X;;echo $?
bash: line 1: bad substitution: no closing `)' in a $((1)) $((2

1
status 0
-- cat <<X;;${x:-a;;X
bash: line 1: bad substitution: no closing `}' in ${x:-a

status 1
-- cat <<X;;a ${x:;;X
bash: line 1: bad substitution: no closing `}' in a ${x:

status 1
-- cat <<X;;a ${#x:-b;;X
bash: line 1: a ${#x:-b
: bad substitution
status 1
-- cat <<X;;a ${!x:-b;;X
bash: line 1: x: invalid indirect expansion
status 1
-- x=y; cat <<X;;a ${!x:-b;;X
bash: line 1: bad substitution: no closing `}' in a ${!x:-b

status 1
-- cat <<X;;a ${x[1]#b;;X
bash: line 1: bad substitution: no closing `}' in a ${x[1]#b

status 1
-- cat <<X;;a ${1%b;;X
bash: line 1: bad substitution: no closing `}' in a ${1%b

status 1
-- cat <<X;;a ${x b:-c;;X
bash: line 1: a ${x b:-c
: bad substitution
status 1
-- cat <<X;;a ${x/b;;X
bash: line 1: bad substitution: no closing `}' in a ${x/b

status 1
-- cat <<X;;a ${x@;;X
bash: line 1: bad substitution: no closing `}' in a ${x@

status 1
-- a=(1); cat <<X;;a ${a[1/0]:-b;;X
bash: line 1: 1/0: division by 0 (error token is "0")
status 1
-- set -u; cat <<X;;a ${u%b;;X
bash: line 1: bad substitution: no closing `}' in a ${u%b

status 1

== status 0
