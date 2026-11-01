== stdout 97
[tab one]
[two tabs val]
[    four spaces]
[ 	space then tab]
<quoted $v>
{	not stripped}
{	EOF}

== stderr 0

== merged 97
[tab one]
[two tabs val]
[    four spaces]
[ 	space then tab]
<quoted $v>
{	not stripped}
{	EOF}

== status 0
