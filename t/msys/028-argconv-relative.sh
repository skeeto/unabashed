# vos
# Relative paths: only their slashes may change
mkdir -p a/b
argv a/b ./a ../x 'a\b' './a\b' 'a/b\c' . .. ./ a/
cd a && argv ../a/b ./b
