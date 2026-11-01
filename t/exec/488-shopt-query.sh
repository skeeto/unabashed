shopt -q extglob; echo "extglob=$?"
shopt -q nullglob; echo "nullglob=$?"
shopt -s nullglob dotglob
shopt -q nullglob dotglob; echo "both=$?"
shopt -q nullglob failglob; echo "mixed=$?"
shopt nullglob dotglob failglob
echo "st=$?"
shopt -p nullglob failglob
echo "st=$?"
shopt -u nullglob
shopt nullglob; echo "st=$?"
shopt -s nosuchopt; echo "st=$?"
shopt -q nosuchopt; echo "q st=$?"
shopt nosuchopt; echo "plain st=$?"
shopt -s -u nullglob; echo "su st=$?"
shopt -x; echo "badflag st=$?"
shopt -so errexit; echo "o st=$? $-"
shopt -uo errexit
shopt -qo errexit; echo "qo=$?"
shopt -o nosuch; echo "o bad st=$?"
shopt -p inherit_errexit shift_verbose
