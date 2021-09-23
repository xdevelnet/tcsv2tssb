.PHONY: all clean
all:
	cc --std=c99 src/main.c -o tcsv2tssb -Wall -I../cccsvparser/src/ -O0
clean:
	rm -f tcsv2tssb
