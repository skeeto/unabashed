== stdout 1113
allexport      	off
braceexpand    	on
emacs          	off
errexit        	off
errtrace       	off
functrace      	off
hashall        	on
histexpand     	off
history        	off
ignoreeof      	off
interactive-comments	on
keyword        	off
monitor        	off
noclobber      	off
noexec         	off
noglob         	off
nolog          	off
notify         	off
nounset        	off
onecmd         	off
physical       	off
pipefail       	off
posix          	off
privileged     	off
verbose        	off
vi             	off
xtrace         	off
---
set +o allexport
set -o braceexpand
set +o emacs
set +o errexit
set +o errtrace
set +o functrace
set -o hashall
set +o histexpand
set +o history
set +o ignoreeof
set -o interactive-comments
set +o keyword
set +o monitor
set +o noclobber
set +o noexec
set +o noglob
set +o nolog
set +o notify
set +o nounset
set +o onecmd
set +o physical
set +o pipefail
set +o posix
set +o privileged
set +o verbose
set +o vi
set +o xtrace
---
braceexpand=on
errexit=on
hashall=on
nounset=on
pipefail=on
posix=off
xtrace=off
set -o errexit
set -o nounset
set -o pipefail
set +o posix

== stderr 0

== merged 1113
allexport      	off
braceexpand    	on
emacs          	off
errexit        	off
errtrace       	off
functrace      	off
hashall        	on
histexpand     	off
history        	off
ignoreeof      	off
interactive-comments	on
keyword        	off
monitor        	off
noclobber      	off
noexec         	off
noglob         	off
nolog          	off
notify         	off
nounset        	off
onecmd         	off
physical       	off
pipefail       	off
posix          	off
privileged     	off
verbose        	off
vi             	off
xtrace         	off
---
set +o allexport
set -o braceexpand
set +o emacs
set +o errexit
set +o errtrace
set +o functrace
set -o hashall
set +o histexpand
set +o history
set +o ignoreeof
set -o interactive-comments
set +o keyword
set +o monitor
set +o noclobber
set +o noexec
set +o noglob
set +o nolog
set +o notify
set +o nounset
set +o onecmd
set +o physical
set +o pipefail
set +o posix
set +o privileged
set +o verbose
set +o vi
set +o xtrace
---
braceexpand=on
errexit=on
hashall=on
nounset=on
pipefail=on
posix=off
xtrace=off
set -o errexit
set -o nounset
set -o pipefail
set +o posix

== status 0
