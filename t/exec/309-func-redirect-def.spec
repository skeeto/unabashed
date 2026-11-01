== stdout 264
read: line one
read: line two
read2: line one
read2: line two
read2: more
read2: more
to stderr
first=line one
first=line one
writer () 
{ 
    echo "line one";
    echo "line two"
} > /tmp/out
reader () 
{ 
    read -r first;
    echo "first=$first"
} < /tmp/out

== stderr 0

== merged 264
read: line one
read: line two
read2: line one
read2: line two
read2: more
read2: more
to stderr
first=line one
first=line one
writer () 
{ 
    echo "line one";
    echo "line two"
} > /tmp/out
reader () 
{ 
    read -r first;
    echo "first=$first"
} < /tmp/out

== status 0
