== stdout 147
f () 
{ 
    cat <<-E
tab
  	space-tab
  tab-space
two-tabs
  E
 E
E

    if true;    then
        cat <<-'X' |
x1
X
  cat <<-Y
y1
 	Y
Y

    fi
}

== stderr 0

== merged 147
f () 
{ 
    cat <<-E
tab
  	space-tab
  tab-space
two-tabs
  E
 E
E

    if true;    then
        cat <<-'X' |
x1
X
  cat <<-Y
y1
 	Y
Y

    fi
}

== status 0
