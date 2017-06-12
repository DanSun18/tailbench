for tid in \
	$(ps --no-headers -ww -p "$1" -L -olwp | sed 's/$/ /' | tr  -d '\n')
	do
	echo $tid
	done
