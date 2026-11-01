== stdout 7
traced

== stderr 63
+ cat /dev/fd/63
++ echo traced
+ read -r v
++ echo 3
+ set +x

== merged 70
+ cat /dev/fd/63
++ echo traced
traced
+ read -r v
++ echo 3
+ set +x

== status 0
