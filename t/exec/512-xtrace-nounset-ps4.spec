== stdout 10
after
end

== stderr 106
bash: line 4: nope: unbound variable
+$nope echo after
bash: line 5: nope: unbound variable
+$nope set +x

== merged 116
bash: line 4: nope: unbound variable
+$nope echo after
after
bash: line 5: nope: unbound variable
+$nope set +x
end

== status 0
