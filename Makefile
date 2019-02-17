all:
	cc --std=c99 src/main.c -o tcsv2tssb -Wall -I../csv_parser/src/ -O0
