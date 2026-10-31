shopt -s expand_aliases
alias fn='function ' nm='realname'
fn nm { echo body; }
function nm { echo plain; }
declare -f
