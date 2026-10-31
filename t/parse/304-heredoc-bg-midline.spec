== stdout 95
f () 
{ 
    cat <<A &
a
A
  echo x
    echo y;
    cat <<A &
a
A
  cat <<B &
b
B
  echo end
}

== stderr 0

== merged 95
f () 
{ 
    cat <<A &
a
A
  echo x
    echo y;
    cat <<A &
a
A
  cat <<B &
b
B
  echo end
}

== status 0
