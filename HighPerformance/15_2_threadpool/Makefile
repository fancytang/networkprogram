TAR1 = web_server
TAR2 = stress_test
TAR3 = my_client
OBJ1 = web_server.o http_conn.o
OBJ2 = stress_test.o
OBJ3 = my_client.o
++ := g++

all: web_server stress_test my_client

$(TAR1) : $(OBJ1)
	$(++) $(OBJ1) -o $(TAR1) -lpthread
$(TAR2): $(OBJ2)
	$(++) $(OBJ2) -o $(TAR2)
$(TAR3): $(OBJ3)
	$(++) $(OBJ3) -o $(TAR3)

$(OBJ1):http_conn.h locker.h thread_pool.h
$(OBJ2):stress_test.cpp
$(OBJ3):my_client.cpp

cleanO:
	rm -rf $(OBJ1) $(OBJ2) $(OBJ3)
cleanE:
	rm -rf $(TAR1) $(TAR2) $(TAR3)
