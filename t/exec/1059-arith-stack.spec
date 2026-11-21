== stdout 39
status 1
pipe 1 0
nested 1
deep 0
done

== stderr 306
bash: line 6: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")
bash: line 7: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")
bash: line 1: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")

== merged 345
bash: line 6: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")
status 1
bash: line 7: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")
pipe 1 0
bash: line 1: a[${c[$((b))]}]: expression recursion level exceeded (error token is "a[${c[$((b))]}]")
nested 1
deep 0
done

== status 0
