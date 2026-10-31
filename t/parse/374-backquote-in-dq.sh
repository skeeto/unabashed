f() {
    echo "a`echo "b c"`d" "`echo \"q\"`" "`echo ')'`" "x`echo \`y\``"
    echo "`echo a # b`" `echo c # d` "`echo ${x:-}`"
}
declare -f
