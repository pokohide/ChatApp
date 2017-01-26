CC := gcc

all: client server member db
client: client.c
	$(CC) -o client client.c

server: server.c
	$(CC) -o server server.c

member: member.c
	$(CC) -o member member.c

db: db.c
	$(CC) -o db db.c

clean:
	$(RM) client server member db
