# vos
# \w and \W abbreviate HOME as ~, also when cd used other case
cd ~
p='\w|\W'
echo "${p@P}"
mkdir -p ~/sub && cd ~/sub && echo "${p@P}"
cd "$(pwd -W)" && echo "${p@P}"
cd / && echo "${p@P}"
