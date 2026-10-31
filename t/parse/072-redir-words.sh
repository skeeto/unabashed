f() {
    a > "$file"
    a > 'q f'
    a > $dir/x.$$
    a < ~/in
    a > ${x:-def}
    a > $(name)
    a 2> /dev/null
    a > a\ b
}
declare -f
