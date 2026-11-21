== stdout 125
"\C-t": "abcdefghijklmnopqrstuvwxyz99999"
"\C-xr" "echo 99999"
set comment-begin #99999
set vi-ins-mode-string ins99999
done

== stderr 0

== merged 125
"\C-t": "abcdefghijklmnopqrstuvwxyz99999"
"\C-xr" "echo 99999"
set comment-begin #99999
set vi-ins-mode-string ins99999
done

== status 0
