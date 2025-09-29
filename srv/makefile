CC = gcc
CFLAGS=-lpthread
OBJ = srv.o

%.o: %.c *.h
	$(CC) -c -o $@ $< $(CFLAGS) 
    
srv: $(OBJ)  
	$(CC) -o $@ $^ $(CFLAGS)

clean :
	rm -rf srv $(OBJ)

new :
	$(MAKE) clean
	$(MAKE)