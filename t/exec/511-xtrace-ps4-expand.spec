== stdout 73
one
two
three 
four
five
six
argv[1] = <eight>
nine
"V" "V" d e 'V'
done

== stderr 395
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
"q" 's' ~ A \ <(:) >(:) * echo six
"q" 's' ~ A \ <(:) >(:) * set +x
>>( recho seven
>>( set +x
>>( recho eight
$((1+1)) A echo nine
$((1+1)) A set +x

== merged 468
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
"q" 's' ~ A \ <(:) >(:) * echo six
six
"q" 's' ~ A \ <(:) >(:) * set +x
>>( recho seven
>>( set +x
>>( recho eight
argv[1] = <eight>
$((1+1)) A echo nine
nine
$((1+1)) A set +x
"V" "V" d e 'V'
done

== status 0
