default: app

context.o: context.s
	gcc -g -m32 -c context.s

thread.o: thread.c
	gcc -g -Werror -m32 -O3 -c thread.c

app: app.c thread.o context.o
	gcc -g -O3 -m32 thread.o context.o -o app app.c

run: app
	./app

test: app
	./app 1024
	./app 1024 1

clean:
	rm -f thread.o app context.o
