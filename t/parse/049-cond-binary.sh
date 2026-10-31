f() {
    [[ a == b ]]; [[ a = b ]]; [[ a != b ]]; [[ a < b ]]; [[ a > b ]]
    [[ 1 -eq 2 ]]; [[ 1 -ne 2 ]]; [[ 1 -lt 2 ]]; [[ 1 -le 2 ]]
    [[ 1 -gt 2 ]]; [[ 1 -ge 2 ]]
    [[ x -nt y ]]; [[ x -ot y ]]; [[ x -ef y ]]
}
declare -f
