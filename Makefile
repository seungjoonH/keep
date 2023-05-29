all: keep

keep: keep.c
	gcc keep.c -o keep -lm

clean:
	rm -f keep
