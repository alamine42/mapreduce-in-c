#include "dict.c"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFSIZE 1024
#define WORKERS 4

int SendAll(int socket, void *buffer, size_t length);
void * AssignToWorker(void * arguments);
void UpdateDictionary(char * encoded_dict);
void InitializeWorkerList();
void PrintWorkerList();
void PrintWorker(int worker_idx);
void Die(char * mess);

struct worker {
  char * ip_addr;
  char * port;
  char * worker_name;
};

struct arg_struct {
  char * ip_addr;
  char * port;
  char * buf;
  size_t bytes_read;
};

Dict WORD_DICT;
struct worker * WORKERS_LIST[WORKERS];
pthread_t tid[WORKERS];
pthread_mutex_t lock;

int main(int argc, char *argv[]) {

  char buffer[BUFFSIZE];
  FILE * fp;

  if (argc != 3) {
    fprintf(stderr, "USAGE: TCPecho <file_name> <threads>\n");
    exit(1);
  }

  /* Initialize word count dictionary */
  WORD_DICT = DictCreate();

  /* Initialize workers */
  InitializeWorkerList();
  // PrintWorkerList();

  /* Open the file containing the data to be transmitted */
  size_t bytesRead = 0;
  fp = fopen(argv[1], "rb");

  if (fp != NULL)    
  {
    fprintf(stdout, "Reading file ...\n\n");
    int workers_assigned = 0;
    int max_workers = atoi(argv[2]);

    while ((bytesRead = fread(buffer, 1, BUFFSIZE - 1, fp)) > 0)
    {
      fprintf(stdout, "Read %zu bytes:\n", bytesRead);
      // fprintf(stdout, "%s\n", buffer);
      fprintf(stdout, "Assigning this chunk to: \n");
      PrintWorker(workers_assigned);
      fprintf(stdout, "\n");

      struct arg_struct args;
      args.ip_addr = WORKERS_LIST[workers_assigned]->ip_addr;
      args.port = WORKERS_LIST[workers_assigned]->port;
      args.buf = buffer;
      args.bytes_read = bytesRead;

      /* Assign this block to the next worker */
      AssignToWorker((void *) &args);
      workers_assigned++;

      /* If we reach max worker utilization, wait and join all threads, before reading further in the file */
      if (workers_assigned == max_workers) {
        workers_assigned = 0;
      }
    }
  }

  fclose(fp);
  exit(0);
}

void PrintWorkerList() {
  int i;
  for (i = 0; i < WORKERS; i++) {
    PrintWorker(i);
  }
}

void PrintWorker(int worker_idx) {
  fprintf(stdout, "Worker %s running at %s:%s\n", 
    WORKERS_LIST[worker_idx]->worker_name, 
    WORKERS_LIST[worker_idx]->ip_addr, 
    WORKERS_LIST[worker_idx]->port);
}

void InitializeWorkerList() {
  struct worker *w0, *w1, *w2, *w3;
  
  w0 = malloc(sizeof(*w0));
  w1 = malloc(sizeof(*w1));
  w2 = malloc(sizeof(*w2));
  w3 = malloc(sizeof(*w3));

  w0->ip_addr = "127.0.0.1";
  w0->port = "8888";
  w0->worker_name = "W0";

  w1->ip_addr = "127.0.0.1";
  w1->port = "8889";
  w1->worker_name = "W1";

  w2->ip_addr = "127.0.0.1";
  w2->port = "8890";
  w2->worker_name = "W2";

  w3->ip_addr = "127.0.0.1";
  w3->port = "8891";
  w3->worker_name = "W3";

  WORKERS_LIST[0] = w0;
  WORKERS_LIST[1] = w1;
  WORKERS_LIST[2] = w2;
  WORKERS_LIST[3] = w3;
}

void * AssignToWorker(void * arguments) {

  struct arg_struct *args = arguments;
  char * buffer = args->buf;
  size_t bytesRead = args->bytes_read;
  char * ip_addr = args->ip_addr;
  char * port = args->port;

  int sock;
  struct sockaddr_in echoserver;
  char * encoded_dict;
  int received = 0;

  /* Create the TCP socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(ip_addr);  /* IP address */
  echoserver.sin_port = htons(atoi(port));       /* server port */

  /* Establish connection */
  if (connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) {
    Die("Failed to connect with server");
  }

  /* Send the payload to the server */
  if (send(sock, buffer, bytesRead, 0) == 0) {
    Die("Mismatch in number of sent bytes");
  }

  close(sock);
  return NULL;
}

void UpdateDictionary(char * encoded_dict) {
  char * token;
  char word[100];
  char word_count[10];
  int wc = 0;

  token = strtok(encoded_dict, ",:");

  while (token != NULL)
  {
    /* Extract the word ... */
    int len = strlen(token);
    strcpy(word, token);
    word[len] = '\0';

    /* And its count */
    token = strtok(NULL, ",:");
    int len2 = strlen(token);
    strcpy(word_count, token);
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

    token = strtok(NULL, ",:");
  }
}

int SendAll(int socket, void *buffer, size_t length) {
    char *ptr = (char*) buffer;
    while (length > 0)
    {
        int i = send(socket, ptr, length, 0);
        if (i < 1) return 0;
        ptr += i;
        length -= i;
    }
    return 1;
}

void Die(char *mess) { perror(mess); exit(1); }