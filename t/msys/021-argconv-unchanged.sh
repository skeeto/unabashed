# vos
# Arguments the classifier leaves alone
argv http://a/b a/b ./a/b ../a '*/x' '/x*' '/x?' '/[x]' 1:2 x:/y C:/x 'C:\x' \
     '~/x' ':/x' "a'b/c" 'a`b/c' '/x::y' 'a/~b' a:./b a:../b /a@@b '' plain \
     -x --flag=word 'a b' 'C:/x;D:/y'
