# fc: listing, -s with substitutions, the editor form, and its errors
cd /tmp
cat > fcs <<'END'
set -o history
echo aa ab ac
echo two
fc -l
fc -nl 1 2
fc -lr -2
fc -l -0
fc -l ec
fc -l nosuch; echo "st $?"
fc -s a=x echo; echo "st $?"
fc -s two
r=x; fc -s nosuch; echo "st $?"
fc -0; echo "st $?"
fc -v; echo "st $?"
fc -e; echo "st $?"
(exit 42)
fc -s; echo "st $?"
fc -e false; echo "st $?"
echo edited
fc -e cat
FCEDIT=cat fc -1
history 4
END
bash fcs 2>&1
