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
#include <pthread.h>
#include <signal.h>

#define MAXPENDING 5    /* Max connection requests */
#define BUFFSIZE 1024

void Die(char * mess);
void * HandleClient(void * sock);
void UpdateDictionary(char * encoded_dict);
void AddToDict(char * buf);
void SigHandler(int signo);

Dict WORD_DICT;
pthread_mutex_t lock;

int main(int argc, char * argv[]) 
{
	int serversock, clientsock;
	struct sockaddr_in echoserver, echoclient;

	if (argc != 2) {
	  fprintf(stderr, "USAGE: reducer <port>\n");
	  exit(1);
	}

	if (signal(SIGTSTP, SigHandler) == SIG_ERR) {
		fprintf(stdout, "\nCan't catch signal.\n");
	}

	if (pthread_mutex_init(&lock, NULL)	!= 0) {
		fprintf(stdout, "\n	Mutex init failed\n");
		return 1;
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

	/* Initialize word count dictionary */
	WORD_DICT = DictCreate();

	/* Run until cancelled */
	while (1) {
		pthread_t tid;
		unsigned int clientlen = sizeof(echoclient);
		/* Wait for client connection */
		if ((clientsock = accept(serversock, (struct sockaddr *) &echoclient, &clientlen)) < 0) {
			Die("Failed to accept client connection");
		}
		fprintf(stdout, "\nClient connected: %s\n", inet_ntoa(echoclient.sin_addr));

		if( pthread_create(&tid, NULL, &HandleClient, (void *) &clientsock) != 0) {
        	Die("Couldn't create thread.");
        }

		fprintf(stdout, "Client handled.\n");
	}

	return 0;
}

void * HandleClient(void * socket) {

	int sock = *(int *)socket;
	int received = -1;
	char * encoded_dict;

	long int raw_dict_size = 0;
	long int true_dict_size = 0;
	int dict_size_bytes = 0;
	if ((dict_size_bytes = recv(sock, &raw_dict_size, sizeof(raw_dict_size), 0)) < 1) {
		Die("Failed to receive size of dictionary.");
	}

	true_dict_size = ntohl(raw_dict_size);

	encoded_dict = (char *) malloc(true_dict_size + 1);
	// Receive the word back from the server 

	int bytes = 0;
	if ((bytes = recv(sock, encoded_dict, true_dict_size, 0)) < 1) {
	    Die("Failed to receive bytes from server");
	}
	fprintf(stdout, "Received %d bytes from worker ... \n", bytes);
	encoded_dict[bytes - 1] = '\0';
	
	pthread_mutex_lock(&lock);

	/* Update the dictionary with the counts received from the worker */
	UpdateDictionary(encoded_dict);

	free(encoded_dict);
	pthread_mutex_unlock(&lock);

	return NULL;
}

void UpdateDictionary(char * enc_dict) {

  char * word_token;
  char word[BUFFSIZE];
  char word_count[BUFFSIZE];
  int wc = 0;

  word_token = strtok(enc_dict, ",:");

  while (word_token != NULL)
  {
    /* Extract the word ... */
    int len = strlen(word_token);
    if (len > BUFFSIZE) {
    	fprintf(stdout, "This word token is really large!\n");
    }
    strcpy(word, word_token);
    word[len] = '\0';

    /* And its count */
    word_token = strtok(NULL, ",:");
    int len2 = strlen(word_token);
    if (len2 > BUFFSIZE) {
    	fprintf(stdout, "This word count is really large!\n");
    }
    strcpy(word_count, word_token);
    word_count[len2] = '\0';
    wc = atoi(word_count);
    // fprintf(stdout, "The word %s shows up %d times.\n", word, wc);

    /* Look up the word in the dict */
    int word_count_in_dict = DictSearch(WORD_DICT, word);

    /* If it's not there, insert it with a the count provided by the worker */
    if (word_count_in_dict == 0) {
      DictInsert(WORD_DICT, word, wc);
    } else { /* Else, get its existing count and add to it the count provided by the worker */
      DictDelete(WORD_DICT, word);
      DictInsert(WORD_DICT, word, word_count_in_dict + wc);
    }

    word_token = strtok(NULL, ",:");
  }
}

void SigHandler(int signo) {
	if (signo == SIGTSTP) {
		/* Print Dict Contents */
		fprintf(stdout, "\nFinal word count:\n\n");
		DictPrint(WORD_DICT);
		DictDestroy(WORD_DICT);
		WORD_DICT = DictCreate();
		fprintf(stdout, "\nDictionary reset.\n\n");
		// exit(0);
	}
}

void Die(char * mess) { 
	perror(mess); 
	exit(1); 
}
