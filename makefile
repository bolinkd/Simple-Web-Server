default: sws
sws: sws.c
	gcc -Wall -o sws sws.c
clean:
	rm sws
