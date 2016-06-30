CC = gcc
FLAGS = -g -Wall -c
worker_OBJECTS = worker.o
driver_OBJECTS = driver.o
reducer_OBJECTS = reducer.o

all: worker.o worker driver.o driver reducer.o reducer clean

worker.o: worker.c
	$(CC) $(CFLAGS) -c worker.c

worker: $(worker_OBJECTS)
	$(CC) $(worker_OBJECTS) -o worker

driver.o: driver.c
	$(CC) $(CFLAGS) -c driver.c

driver: $(driver_OBJECTS)
	$(CC) $(driver_OBJECTS) -o driver

reducer.o: reducer.c
	$(CC) $(CFLAGS) -c reducer.c

reducer: $(reducer_OBJECTS)
	$(CC) $(reducer_OBJECTS) -o reducer

clean:
	rm -f *.o