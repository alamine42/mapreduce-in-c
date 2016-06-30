#include "dict.c"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define MAXPENDING 5    /* Max connection requests */
#define BUFFSIZE 1024
#define REDUCER_IP "127.0.0.1"
#define REDUCER_PORT "5555"

Dict WORD_DICT;

void Die(char * mess);
void HandleClient(int sock);
void NormalizeText(char *p);
void AddToDict(char * buf);

int main(int argc, char * argv[]) 
{
	int serversock, clientsock;
	struct sockaddr_in echoserver, echoclient;

	if (argc != 2) {
	  fprintf(stderr, "USAGE: echoserver <port>\n");
	  exit(1);
	}
	/* Create the TCP socket */
	if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		Die("Failed to create socket");
	}
	/* Construct the server sockaddr_in structure */
	memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
	echoserver.sin_family = AF_INET;                  /* Internet/IP */
	echoserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
	echoserver.sin_port = htons(atoi(argv[1]));       /* server port */

	/* Bind the server socket */
	if (bind(serversock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) {
		Die("Failed to bind the server socket");
	}
	/* Listen on the server socket */
	if (listen(serversock, MAXPENDING) < 0) {
		Die("Failed to listen on server socket");
	}

	/* Run until cancelled */
	while (1) {
		/* Initialize word count dictionary */
		WORD_DICT = DictCreate();

		unsigned int clientlen = sizeof(echoclient);
		/* Wait for client connection */
		if ((clientsock = accept(serversock, (struct sockaddr *) &echoclient, &clientlen)) < 0) {
			Die("Failed to accept client connection");
		}
		fprintf(stdout, "\nClient connected: %s\n", inet_ntoa(echoclient.sin_addr));
		HandleClient(clientsock);
		fprintf(stdout, "Client handled.\n");

		/* Destroy word count dictionary and free up memory */
		DictDestroy(WORD_DICT);
	}
}

void HandleClient(int sock) {
	char buffer[BUFFSIZE];
	int received = -1;
	char * dict_rep;

	/* Receive message */
	if ((received = recv(sock, buffer, BUFFSIZE - 1, 0)) < 0) {
		Die("Failed to receive initial bytes from Driver.");
	}

	/* Do the following while data is still coming in */
	if (received > 0) {

		/* Gets the bytes received, add a terminating null byte */
		fprintf(stdout, "Received %d bytes from Driver ...\n", received);		
		buffer[received] = '\0';
	
		// fprintf(stdout, "Received this: \n");
		// fprintf(stdout, "%s\n", buffer);

		fprintf(stdout, "Normalize data and counting words ...\n");
		/* Normalizer string by removing punctuation and lower casing */
		NormalizeText(buffer);

		/* and then send off for insertion into word count */
		AddToDict(buffer);
		// fprintf(stdout, "Added to dictionary. Checking if more data is received...\n");
	}

	close(sock);

	dict_rep = DictStringEncode(WORD_DICT);

	int reducer_sock;  
	struct sockaddr_in reducer;

	/* Create the TCP socket */
	if ((reducer_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		Die("Failed to create socket");
	}

	/* Construct the server sockaddr_in structure */
	memset(&reducer, 0, sizeof(reducer));       /* Clear struct */
	reducer.sin_family = AF_INET;                  /* Internet/IP */
	reducer.sin_addr.s_addr = inet_addr(REDUCER_IP);  /* IP address */
	reducer.sin_port = htons(atoi(REDUCER_PORT));       /* server port */

	/* Establish connection */
	if (connect(reducer_sock, (struct sockaddr *) &reducer, sizeof(reducer)) < 0) {
		Die("Failed to connect with server");
	}

	/* Tell the driver the size of the encoded dict that we are going to send it */
	long int encoded_dict_length = strlen(dict_rep);

	int converted_length = htonl(encoded_dict_length);
	if (send(reducer_sock, &converted_length, sizeof(converted_length), 0) == 0) {
		Die("Failed to send size of encoded dict.");
	}
	fprintf(stdout, "Sending a dict string of size %lu\n", encoded_dict_length);

	/* Now send the actual encoded dict */
	if (send(reducer_sock, dict_rep, encoded_dict_length, 0) == 0) {
		Die("Failed to send bytes to client");
	}

	free(dict_rep);
}

void AddToDict(char * buf) {
	char * token, * cp;

	token = strtok(buf, " \n");
	while (token != NULL)
	{
		int len = strlen(token);
		token[len] = '\0';

		// printf ("%s is a string of length %d\n", token, len);
		int token_value = DictSearch(WORD_DICT, token);
		if (token_value == 0) {
			// fprintf(stdout, "Word is not in dictionary. Adding it. \n");
			DictInsert(WORD_DICT, token, 1);
		} else {
			// fprintf(stdout, "Word already exists!\n");
			DictDelete(WORD_DICT, token);
			DictInsert(WORD_DICT, token, token_value + 1);
		}
		token = strtok (NULL, " \n");
	}
}

void NormalizeText(char * p) {

    char *src = p, *dst = p;

    while (*src)
    {
       if (ispunct((unsigned char)*src))
       {
          /* Skip this character */
          src++;
       }
       else if (isupper((unsigned char)*src))
       {
          /* Make it lowercase */
          *dst++ = tolower((unsigned char)*src);
          src++;
       }
       else if (src == dst)
       {
          /* Increment both pointers without copying */
          src++;
          dst++;
       }
       else if ((unsigned char)*src == '\n')
       {
       	  *dst++ = ' ';
       	  src++;
       }
       else
       {
          /* Copy character */
          *dst++ = *src++;
       }
    }

    *dst = 0;
}

void Die(char * mess) { 
	perror(mess); 
	exit(1); 
}
