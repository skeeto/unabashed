echo start
(echo "${x!}"; echo after-a); echo "st=$?"
(echo ${:}; echo after-b); echo "st=$?"
(echo "${x y}"; echo after-c); echo "st=$?"
(echo ${#x:1}; echo after-d); echo "st=$?"
(x=1; echo "${x:?}"; echo "${1x}"; echo after-e); echo "st=$?"
(: ${%}; echo after-f); echo "st=$?"
(x=abc; echo "[${x:}]"; echo after-g); echo "st=$?"
(x=abc; echo "[${x: }]"; echo after-h); echo "st=$?"
(set -- a b; echo "[${@: }]"; echo after-i); echo "st=$?"
echo "${!}" 
echo "bang st=$?"
echo "${@:}" 
echo end
