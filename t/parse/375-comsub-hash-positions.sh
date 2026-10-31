f() {
    echo $(echo a#b c# #d )
    ) $(echo ${#}#) $(echo x;#y
    ) $(echo $'#'z \#w)
}
declare -f
