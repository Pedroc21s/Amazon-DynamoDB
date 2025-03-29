# Grupo SD-0 integrado por:
#      Pedro Correia fc59791
#      Sara Ribeiro fc59873
#	   Miguel Mendes fc59885

#all: criar (pelo menos) os targets libtable, client_hashtable e server_hashtable;
#• libtable: Compilar os ficheiros block.c,  entry.c, list.c, e table.c, criando os respetivos
#ficheiro objeto, e colocando-os numa biblioteca estática libtable.a (usar o comando ar com as opções  -r, -c e -s:  ar -rcs libtable.a block.o entry.o list.o table.o. Caso sejam usados os
#ficheiros objeto da 1ª fase do projeto disponibilizados aos alunos, deverão ser estes a ser colocados na
#biblioteca;
#• client_hashtable: Compilar todo o código que pertence ao cliente, criando a aplicação
#client_hashtable (ligar todos os ficheiros objetos necessários e a biblioteca libtable.a);
#• server_hashtable: Compilar todo o código que pertence ao servidor, criando a aplicação
#server_hashtable ligar todos os ficheiros objetos necessários e a biblioteca libtable.a);
#• clean: Remover todos os ficheiros criados pelos targets acima. Nota: se forem usados os ficheiros objeto da
#1ª fase do projeto disponibilizados aos alunos, estes não precisam de ser removidos pelo clean.

CC = gcc
CFLAGS = -Wall -g -I include -D THREADED

OBJ_DIR = object
LIB_DIR = lib
SOURCE_DIR = source
BIN_DIR = binary
HEADER_DIR = include

PROTOC = protoc-c

LIB = $(LIB_DIR)/libtable.a

LIB_SOURCES = block.c entry.c list.c table.c
LIB_OBJECTS = block.o entry.o list.o table.o
block.o = block.c block.h
entry.o = entry.c entry.h block.h
list.o = list.c list.h block.h entry.h list-private.h
table.o = table.c table.h block.h table-private.h
server_network_private.o = server_network_private.c table.h server_network.h server_skeleton.h
server_skeleton_private.o = server_skeleton_private.c server_skeleton.h table.h htmessages.pb-c.h client_stub.h

htmessages.pb-c.o = htmessages.pb-c.c htmessages.pb-c.h
message.o = message-private.h

client_stub.o = client_stub.c client_stub.h client_stub-private.h block.h entry.h htmessages.pb-c.h server_skeleton.h
client_hashtable.o = client_hashtable.c client_stub.h client_stub-private.h block.h entry.h 
client_network.o = client_network.c client_network.h client_stub-private.h message-private.h message.c client_stub.h htmessages.pb-c.h
CLIENT_OBJECTS = client_hashtable.o client_stub.o client_network.o htmessages.pb-c.o

server_skeleton.o = server_skeleton.h table.h entry.h block.h server_network.h  client_stub.h 
server_network.o = server_network.h htmessages.pb-c.h  message-private.h message.c server_skeleton.h
server_hashtable.o = server_hashtable.h server_skeleton.h table.h server_network.h  
SERVER_OBJECTS = server_skeleton.o server_network.o server_hashtable.o htmessages.pb-c.o client_stub.o server_network_private.o server_skeleton_private.o client_network.o

all: $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) libtable initialize_proto client_hashtable server_hashtable

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
$(BIN_DIR):
	mkdir -p $(BIN_DIR)
$(LIB_DIR):
	mkdir -p $(LIB_DIR)

initialize_proto: htmessages.proto
	$(PROTOC) --c_out=$(HEADER_DIR) htmessages.proto
	mv $(HEADER_DIR)/htmessages.pb-c.c $(SOURCE_DIR)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/htmessages.pb-c.o -c $(SOURCE_DIR)/htmessages.pb-c.c 

libtable: $(LIB_OBJECTS)
	ar -rcs $(LIB) $(addprefix $(OBJ_DIR)/,$(LIB_OBJECTS))

%.o: $(SOURCE_DIR)/%.c $($@)
	 $(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ -c $<

client_hashtable: $(CLIENT_OBJECTS) $(LIB)
	$(CC) $(CFLAGS) $(addprefix $(OBJ_DIR)/,$(CLIENT_OBJECTS)) -o $(BIN_DIR)/client_hashtable -L$(LIB_DIR) -ltable -lprotobuf-c -lzookeeper_mt

server_hashtable: $(SERVER_OBJECTS) $(LIB)
	$(CC) $(CFLAGS) $(addprefix $(OBJ_DIR)/,$(SERVER_OBJECTS)) -o $(BIN_DIR)/server_hashtable -L$(LIB_DIR) -ltable -lprotobuf-c -lzookeeper_mt

clean:
	rm -f $(OBJ_DIR)/*.o out
	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/*
