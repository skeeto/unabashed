W=Aaxb
cat <<E
${W%a$'x'b}
${W%a$'\x78'b}
${W/a$'\x78'/Q}
${W/a/$'\x41'}
${W#$'\x41'}
${none-a$'\x41'b}
"${W%a$'\x78'b}"
E
