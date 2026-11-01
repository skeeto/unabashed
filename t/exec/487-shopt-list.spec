== stdout 3116
array_expand_once   	off
assoc_expand_once   	off
autocd              	off
bash_source_fullpath	off
cdable_vars         	off
cdspell             	off
checkhash           	off
checkjobs           	off
checkwinsize        	on
cmdhist             	on
compat31            	off
compat32            	off
compat40            	off
compat41            	off
compat42            	off
compat43            	off
compat44            	off
complete_fullquote  	on
direxpand           	off
dirspell            	off
dotglob             	off
execfail            	off
expand_aliases      	off
extdebug            	off
extglob             	off
extquote            	on
failglob            	off
force_fignore       	on
globasciiranges     	on
globskipdots        	on
globstar            	off
gnu_errfmt          	off
histappend          	off
histreedit          	off
histverify          	off
hostcomplete        	on
huponexit           	off
inherit_errexit     	off
interactive_comments	on
lastpipe            	off
lithist             	off
localvar_inherit    	off
localvar_unset      	off
login_shell         	off
mailwarn            	off
no_empty_cmd_completion	off
nocaseglob          	off
nocasematch         	off
noexpand_translation	off
nullglob            	off
patsub_replacement  	on
progcomp            	on
progcomp_alias      	off
promptvars          	on
restricted_shell    	off
shift_verbose       	off
sourcepath          	on
varredir_close      	off
xpg_echo            	off
---
shopt -u array_expand_once
shopt -u assoc_expand_once
shopt -u autocd
shopt -u bash_source_fullpath
shopt -u cdable_vars
shopt -u cdspell
shopt -u checkhash
shopt -u checkjobs
shopt -s checkwinsize
shopt -s cmdhist
shopt -u compat31
shopt -u compat32
shopt -u compat40
shopt -u compat41
shopt -u compat42
shopt -u compat43
shopt -u compat44
shopt -s complete_fullquote
shopt -u direxpand
shopt -u dirspell
shopt -u dotglob
shopt -u execfail
shopt -u expand_aliases
shopt -u extdebug
shopt -u extglob
shopt -s extquote
shopt -u failglob
shopt -s force_fignore
shopt -s globasciiranges
shopt -s globskipdots
shopt -u globstar
shopt -u gnu_errfmt
shopt -u histappend
shopt -u histreedit
shopt -u histverify
shopt -s hostcomplete
shopt -u huponexit
shopt -u inherit_errexit
shopt -s interactive_comments
shopt -u lastpipe
shopt -u lithist
shopt -u localvar_inherit
shopt -u localvar_unset
shopt -u login_shell
shopt -u mailwarn
shopt -u no_empty_cmd_completion
shopt -u nocaseglob
shopt -u nocasematch
shopt -u noexpand_translation
shopt -u nullglob
shopt -s patsub_replacement
shopt -s progcomp
shopt -u progcomp_alias
shopt -s promptvars
shopt -u restricted_shell
shopt -u shift_verbose
shopt -s sourcepath
shopt -u varredir_close
shopt -u xpg_echo
---
checkwinsize        	on
cmdhist             	on
complete_fullquote  	on
extquote            	on
force_fignore       	on
globasciiranges     	on
globskipdots        	on
hostcomplete        	on
interactive_comments	on
patsub_replacement  	on
progcomp            	on
promptvars          	on
sourcepath          	on
---
unset count nonzero: 1
errexit off
pipefail off
set +o errexit
set +o noclobber

== stderr 0

== merged 3116
array_expand_once   	off
assoc_expand_once   	off
autocd              	off
bash_source_fullpath	off
cdable_vars         	off
cdspell             	off
checkhash           	off
checkjobs           	off
checkwinsize        	on
cmdhist             	on
compat31            	off
compat32            	off
compat40            	off
compat41            	off
compat42            	off
compat43            	off
compat44            	off
complete_fullquote  	on
direxpand           	off
dirspell            	off
dotglob             	off
execfail            	off
expand_aliases      	off
extdebug            	off
extglob             	off
extquote            	on
failglob            	off
force_fignore       	on
globasciiranges     	on
globskipdots        	on
globstar            	off
gnu_errfmt          	off
histappend          	off
histreedit          	off
histverify          	off
hostcomplete        	on
huponexit           	off
inherit_errexit     	off
interactive_comments	on
lastpipe            	off
lithist             	off
localvar_inherit    	off
localvar_unset      	off
login_shell         	off
mailwarn            	off
no_empty_cmd_completion	off
nocaseglob          	off
nocasematch         	off
noexpand_translation	off
nullglob            	off
patsub_replacement  	on
progcomp            	on
progcomp_alias      	off
promptvars          	on
restricted_shell    	off
shift_verbose       	off
sourcepath          	on
varredir_close      	off
xpg_echo            	off
---
shopt -u array_expand_once
shopt -u assoc_expand_once
shopt -u autocd
shopt -u bash_source_fullpath
shopt -u cdable_vars
shopt -u cdspell
shopt -u checkhash
shopt -u checkjobs
shopt -s checkwinsize
shopt -s cmdhist
shopt -u compat31
shopt -u compat32
shopt -u compat40
shopt -u compat41
shopt -u compat42
shopt -u compat43
shopt -u compat44
shopt -s complete_fullquote
shopt -u direxpand
shopt -u dirspell
shopt -u dotglob
shopt -u execfail
shopt -u expand_aliases
shopt -u extdebug
shopt -u extglob
shopt -s extquote
shopt -u failglob
shopt -s force_fignore
shopt -s globasciiranges
shopt -s globskipdots
shopt -u globstar
shopt -u gnu_errfmt
shopt -u histappend
shopt -u histreedit
shopt -u histverify
shopt -s hostcomplete
shopt -u huponexit
shopt -u inherit_errexit
shopt -s interactive_comments
shopt -u lastpipe
shopt -u lithist
shopt -u localvar_inherit
shopt -u localvar_unset
shopt -u login_shell
shopt -u mailwarn
shopt -u no_empty_cmd_completion
shopt -u nocaseglob
shopt -u nocasematch
shopt -u noexpand_translation
shopt -u nullglob
shopt -s patsub_replacement
shopt -s progcomp
shopt -u progcomp_alias
shopt -s promptvars
shopt -u restricted_shell
shopt -u shift_verbose
shopt -s sourcepath
shopt -u varredir_close
shopt -u xpg_echo
---
checkwinsize        	on
cmdhist             	on
complete_fullquote  	on
extquote            	on
force_fignore       	on
globasciiranges     	on
globskipdots        	on
hostcomplete        	on
interactive_comments	on
patsub_replacement  	on
progcomp            	on
promptvars          	on
sourcepath          	on
---
unset count nonzero: 1
errexit off
pipefail off
set +o errexit
set +o noclobber

== status 1
