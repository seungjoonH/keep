all: keep

mxxd: keep.c
	gcc keep.c -o keep

clean:
	rm -f keep