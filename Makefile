SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
CC = gcc
USER = $(shell id -u -n)
CFLAGS = -Wall -ggdb3

default:	$(OBJ_DIR) cassini  saturnd

#compile toutes les cibles
all: $(OBJ_DIR) cassini saturnd


$(OBJ_DIR): 
	@mkdir -p $(OBJ_DIR)



## Cible Cassini

#remove fil-system when tests will be done, or when saturnd will be add and use id
#SRC_CASSINI= $(SRC_DIR)/cassini.c $(SRC_DIR)/commandline.c $(SRC_DIR)/string-utils.c $(SRC_DIR)/path.c $(SRC_DIR)/timing-text-io.c $(SRC_DIR)/request_reply.c $(SRC_DIR)/file-system.c
OBJ_CASSINI= $(OBJ_DIR)/cassini.o $(OBJ_DIR)/commandline.o $(OBJ_DIR)/string-utils.o $(OBJ_DIR)/path.o $(OBJ_DIR)/timing-text-io.o $(OBJ_DIR)/request_reply.o $(OBJ_DIR)/file-system.o

cassini:	$(OBJ_CASSINI)
	$(CC) $(CFLAGS) $(OBJ_CASSINI) -o cassini

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I $(INC_DIR) -o $@ -c $<


## Cible Saturnd

saturnd:	$(OBJ_DIR)/saturnd.o $(OBJ_DIR)/path.o $(OBJ_DIR)/file-system.o $(OBJ_DIR)/commandline.o $(OBJ_DIR)/string-utils.o
	$(CC) $(CFLAGS) -pthread -I $(INC_DIR)  $(OBJ_DIR)/saturnd.o $(OBJ_DIR)/path.o $(OBJ_DIR)/file-system.o $(OBJ_DIR)/commandline.o $(OBJ_DIR)/string-utils.o -o saturnd


#clean fichiers compilés
distclean:
	# supprime les pipes par défaut
	rm -rf cassini saturnd $(OBJ_DIR)/*  || true
	
	
# clean pipes
pathclean:
	rm -r /tmp/$(USER)/saturnd/pipes/*

sfclean:
	rm -r .sysfile

# clean executables
eclean:
	rm cassini
#saturnd
