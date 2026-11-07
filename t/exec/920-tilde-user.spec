== stdout 178
/root /root/x /home/oracle ~nosuch /usr/sbin/ /nonexistent
/root:/home/oracle
a:/root/bin
~root ~root ~root
~root /root
match
case
from BASH_ENV
body
/home/oracle /home/oracle/x

== stderr 0

== merged 178
/root /root/x /home/oracle ~nosuch /usr/sbin/ /nonexistent
/root:/home/oracle
a:/root/bin
~root ~root ~root
~root /root
match
case
from BASH_ENV
body
/home/oracle /home/oracle/x

== status 0
