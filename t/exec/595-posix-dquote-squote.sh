set -o posix
echo "foo ${IFS+'bar} baz"
echo "${IFS+'a'b'}"
x=abc
echo "${x#'a'}" "${x%'c'}"
set +o posix
echo "${IFS+'bar'} baz"
