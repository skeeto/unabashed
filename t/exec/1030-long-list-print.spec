== stdout 548
f 400 400 401
800
f () 
{ 
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j
&& :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    wait
}
2409
k=$(printf '%s\n' $((i+1)); i=$((i+1)); j=$((j+1)) && :; ! false || x; i=$((i+1)); j=$((j+
same
f 400 400 401
800
debug 400

== stderr 0

== merged 548
f 400 400 401
800
f () 
{ 
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j
&& :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    i=$((i+1));
    j=$((j+1)) && :;
    ! false || x;
    wait
}
2409
k=$(printf '%s\n' $((i+1)); i=$((i+1)); j=$((j+1)) && :; ! false || x; i=$((i+1)); j=$((j+
same
f 400 400 401
800
debug 400

== status 0
