# replacement strings: slashes, quoting, patsub_replacement &
p() { printf '<%s>' "$@"; echo; }
x=a/b/c
p "${x/\//-}" "${x//\//-}" "${x//'/'/+}" "${x/b/x/y}"
y=hello
p "${y/l/[&]}" "${y//l/<&>}" "${y/l/\&}" "${y/l/'&'}" "${y/l/"&"}"
p ${y//l/[&]} ${y/l/\&}
amp='&&'; p "${y/e/$amp}" "${y/e/"$amp"}"
p "${y/ll/'x y'}" "${y/ll/"x y"}" ${y/ll/'x y'}
p "${y/e/\\}" "${y/e/\}}" "${y/e/\\&}" ${y/e/\\&}
shopt -u patsub_replacement
p "${y/l/[&]}"
