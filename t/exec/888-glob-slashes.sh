# Globbing a directory part that ends in a run of slashes: the run is
# one slash in the results, however long
mkdir -p abc/d
: > abc/f
echo a*c////f a*c//f a*c///
echo a*c//*
s=$(printf '%02000d' 0)
s=${s//0//}
eval "echo a*c${s}f"
eval "echo a*c${s}d${s}"
rm -r abc
