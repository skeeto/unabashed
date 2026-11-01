== stdout 32
one
two
three
four
five
word in

== stderr 122
bash: -c: line 19: syntax error near unexpected token `)'
bash: -c: line 19: `case esac in esac) echo "word esac";; esac'

== merged 154
one
two
three
four
five
word in
bash: -c: line 19: syntax error near unexpected token `)'
bash: -c: line 19: `case esac in esac) echo "word esac";; esac'

== status 2
