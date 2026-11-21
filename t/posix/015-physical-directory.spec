== stdout 362
cd link: pwd=/link P=/real
-LP /real -PL /link
cd sub: pwd=/link/sub P=/real/sub
cd ..: pwd=/link P=/real
cd -P .: pwd=/real P=/real
cd -P .. from link/sub: pwd=/real P=/real
two links: pwd=/link/up P=/real/sub
cd -P through two links: pwd=/real P=/real
set -P: pwd=/real P=/real
/real
set -o physical: pwd=/real/sub P=/real/sub
posix PWD /real
PWD /link
loop 1

== stderr 0

== merged 362
cd link: pwd=/link P=/real
-LP /real -PL /link
cd sub: pwd=/link/sub P=/real/sub
cd ..: pwd=/link P=/real
cd -P .: pwd=/real P=/real
cd -P .. from link/sub: pwd=/real P=/real
two links: pwd=/link/up P=/real/sub
cd -P through two links: pwd=/real P=/real
set -P: pwd=/real P=/real
/real
set -o physical: pwd=/real/sub P=/real/sub
posix PWD /real
PWD /link
loop 1

== status 0
