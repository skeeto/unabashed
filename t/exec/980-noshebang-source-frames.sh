# A script without #! run from a file a function sources sees only its
# own call frames, as bash's reinitialized child: no "source" or caller
# in BASH_SOURCE, BASH_LINENO or FUNCNAME. The caller's are back after.
mkdir -p d
cat > d/frames <<'END'
echo "source: ${BASH_SOURCE[*]} (${#BASH_SOURCE[@]})"
echo "lineno: ${BASH_LINENO[*]} (${#BASH_LINENO[@]})"
echo "funcname: ${FUNCNAME[*]} (${#FUNCNAME[@]})"
echo "argv: ${BASH_ARGV[*]} argc: ${BASH_ARGC[*]}"
g() { echo "in g: ${BASH_SOURCE[*]} / ${BASH_LINENO[*]} / ${FUNCNAME[*]}"; }
g
END
chmod +x d/frames
printf 'd/frames a b\necho "back: ${BASH_SOURCE[*]} / ${BASH_LINENO[*]} / ${FUNCNAME[*]}"\n' > d/sourced
f() {
  . d/sourced x y
}
f
. d/sourced
d/frames
