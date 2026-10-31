f() {
	cat <<-E
	tab
  	space-tab
	  tab-space
		two-tabs
  E
	 E
	E
	if true; then
		cat <<-'X' | cat <<-Y
			x1
		X
			y1
	 	Y
		Y
	fi
}
declare -f
