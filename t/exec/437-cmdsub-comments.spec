== stdout 75
first
second
<heredoc line>
<indented>
$not ) expanded
a # not comment
a#b

== stderr 0

== merged 75
first
second
<heredoc line>
<indented>
$not ) expanded
a # not comment
a#b

== status 0
