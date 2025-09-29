CC = gcc
CFLAGS=-lpthread
OBJ = cli.o

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS) 
    
cli: $(OBJ)  
	$(CC) -o $@ $^ $(CFLAGS)

clean :
	rm -rf cli $(OBJ)

new :
	$(MAKE) clean
	$(MAKE)