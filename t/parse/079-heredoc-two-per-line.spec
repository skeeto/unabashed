== stdout 88
f () 
{ 
    cat <<A
first
A

    cat <<B
second
B

    a <<X 3<<Y
x body
X
y body
Y

}

== stderr 0

== merged 88
f () 
{ 
    cat <<A
first
A

    cat <<B
second
B

    a <<X 3<<Y
x body
X
y body
Y

}

== status 0
