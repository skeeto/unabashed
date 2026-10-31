f() {
    function a-b { :; }
    function x=y { :; }
    function 9 { :; }
    g() for i in 1; do :; done
    h() (( 1 ))
}
declare -f f
