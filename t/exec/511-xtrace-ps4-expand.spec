== stdout 30
one
two
three 
four
five
done

== stderr 245
+ sub echo one
+ sub set +x
+ dflt echo two
+ dflt set +x
+init echo three ''
+init set +x
+ 2 echo four
+ 2 set +x
bash: line 19: + ${bad x} : bad substitution
+ ${bad x} echo five
bash: line 20: + ${bad x} : bad substitution
+ ${bad x} set +x

== merged 275
+ sub echo one
one
+ sub set +x
+ dflt echo two
two
+ dflt set +x
+init echo three ''
three 
+init set +x
+ 2 echo four
four
+ 2 set +x
bash: line 19: + ${bad x} : bad substitution
+ ${bad x} echo five
five
bash: line 20: + ${bad x} : bad substitution
+ ${bad x} set +x
done

== status 0
