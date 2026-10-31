== stdout 141
f () 
{ 
    cat <<A |
apple
A
  sed s/a/b/ | tr a b <<B
banana
B

    cat <<EOF |
one
EOF
  while read l;    do
        echo $l;
    done
}

== stderr 0

== merged 141
f () 
{ 
    cat <<A |
apple
A
  sed s/a/b/ | tr a b <<B
banana
B

    cat <<EOF |
one
EOF
  while read l;    do
        echo $l;
    done
}

== status 0
