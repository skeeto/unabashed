== stdout 48
f () 
{ 
    cat <<-A <<B
a body
A
	b body
B

}

== stderr 0

== merged 48
f () 
{ 
    cat <<-A <<B
a body
A
	b body
B

}

== status 0
