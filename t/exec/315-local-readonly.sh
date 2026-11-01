f() {
    local -r lr=locked
    echo "lr=$lr"
    lr=changed
    echo "status $? lr=$lr"
    unset lr
    echo "unset status $?"
}
f
echo "outside: [${lr-unset}]"
lr=free
echo "global lr=$lr"
readonly G=1
g() { local G=2; echo "local G status $? G=$G"; }
g
h() { local -r G2=1; inner; }
inner() { G2=2; echo "inner status $? G2=$G2"; }
h
