== stdout 627
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
BASH_FUNC_h%%=() { echo h; }; echo inj
BASH_FUNC_h%%=() {  echo new
--
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
bash: warning: h: ignoring function definition attempt
bash: error importing function definition for `h'
new
--
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
BASH_FUNC_h%%=() { echo h; }; echo inj

== stderr 0

== merged 627
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
BASH_FUNC_h%%=() { echo h; }; echo inj
BASH_FUNC_h%%=() {  echo new
--
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
bash: warning: h: ignoring function definition attempt
bash: error importing function definition for `h'
new
--
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
BASH_FUNC_h%%=() { echo h; }; echo inj

== status 0
