# PS4 comes from the environment (except for root); IFS never does
PS4='+env ' bash -xc ': one' 2>&1
PS4= bash -xc ': two' 2>&1
IFS=x bash -c 'printf "<%s>" "$IFS" | od -c | head -1'
env -u PS4 bash -xc ': three' 2>&1
