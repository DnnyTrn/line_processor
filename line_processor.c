#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#define INPUTLINE_LENGTH 1000
#define OUTPUTLINE_LENGTH 80
#define BUFFERSIZE 10
// buffer of shared by input and processor threads
char buffer01[BUFFERSIZE][INPUTLINE_LENGTH+1] = {""};
size_t bufferCount01 = 0;        // buffer index
size_t fill_ptr = 0;
size_t use_ptr = 0;

// Initialize the mutex
pthread_mutex_t mutex;

// Initialize the condition variables
pthread_cond_t full, empty;

// replace every instance of ++ with ^

void* lineSeparator(void* args);
// output to standard out
void * sendOut(void*arg){
    char* line;
    while(1){
        pthread_mutex_lock(&mutex);

        // sleep thread if buffer is empty, wait for full signal
        while(bufferCount01 == 0)
            pthread_cond_wait(&full, &mutex);

        line = buffer01[use_ptr];
        printf("%zu: %s", use_ptr, line);
        use_ptr = (use_ptr + 1) % BUFFERSIZE;
        bufferCount01--;

        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}
//Input Thread(producer only): This thread collects input on a line-by-line basis from standard input.
void* getInput(void*args){
    char line[INPUTLINE_LENGTH+1] = "";
    // store line into buffer array (produce item)
    while(fgets(line, INPUTLINE_LENGTH+1, stdin) != NULL){
        pthread_mutex_lock(&mutex);

        // sleep producer if buffer is full, wait for empty signal
        while(bufferCount01 == BUFFERSIZE)
            pthread_cond_wait(&empty, &mutex);

        strcpy(buffer01[fill_ptr], line);
        fill_ptr = (fill_ptr + 1) % BUFFERSIZE;
        bufferCount01++;

        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);
    }
    exit(0);

    // terminate program on DONE\n
    //    char * done = strstr(input, "DONE\n");
    //     if(done && strlen(input) == 5) break;
        
        // process line before printing

    // pthread_cond_signal(&full);
      // Signal to the consumer that the buffer is no longer empty
      // Unlock the mutex

    return NULL;
} 


int main(void){
    assert(pthread_mutex_init(&mutex, NULL) == 0);
    assert(pthread_cond_init(&full, NULL) == 0);
    assert(pthread_cond_init(&empty, NULL) == 0);
    pthread_t thread[4];
    pthread_create(&thread[0], NULL, getInput, NULL);
    // pthread_create(&thread[1], NULL, lineSeparator, NULL);
    pthread_create(&thread[2], NULL, sendOut, NULL);
    
    pthread_join(thread[0], NULL);
    // pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&full);
    return 0;
}

// Thread function that converts the line separator into a space
// shares buffer01 with t1/getInput()
void* lineSeparator(void* args){
    
    while(bufferCount01 > 0){
        pthread_mutex_lock(&mutex);
        buffer01[0][strcspn(buffer01[0], "\n")] = ' ';
        bufferCount01--;
        pthread_mutex_unlock(&mutex);
        if(bufferCount01 < BUFFERSIZE) 
            pthread_cond_signal(&empty);
    }
           // check for partial line (lines that have less than 80 chars)
        // if(strchr(input, '\n') != NULL){    //partial lines contain a newline char
        //     input[strcspn(input, "\n")] = ' ';  // replace newline with space
        //     strcpy(beginning, input);   //partial line will be printed in next iteration of fgets 
        //     n -= strlen(input);     //offset the next fgets by n - length of partial line
        //     continue;
        // }
}

void processString(char *s)
{
    size_t len = strlen(s), i,j;
    if(len < 1) perror("string is length 0\n");
    
    //  Every adjacent pair of plus signs, i.e., "++", is replaced by a "^".
    for (i = 0; i < len; i++)
    {
        if (s[i] == '+' && s[i + 1] == '+'){
            s[i] = '^';
            i++; 
        }
    }
}
