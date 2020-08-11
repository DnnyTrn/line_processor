#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#define INPUTLINE_LENGTH 1000
#define OUTPUTLINE_LENGTH 80
#define BUFFERSIZE 10
#define NUM_BUFFERS 3
#define END_MARKER "@@"
int init();
int destroy();
void* lineSeparator(void* args);
// buffer of shared by input and processor threads
// char buffer[NUM_BUFFERS][BUFFERSIZE][INPUTLINE_LENGTH+1];
typedef struct {
    char input[BUFFERSIZE][INPUTLINE_LENGTH+1];
    pthread_mutex_t mutex;
    size_t count, fill_ptr, use_ptr; 
}Buffer;
Buffer buffer_array[NUM_BUFFERS] = {0};

pthread_cond_t full, empty;

// replace every instance of ++ with ^

// output to standard out
void * sendOut(void*arg){
    Buffer * buffer = &buffer_array[1];
    size_t * use_ptr = &buffer->use_ptr;    //consumer's index to buffer0
    char* line = buffer->input[*use_ptr];
    
    while(strcmp(line, END_MARKER) != 0){
     
        pthread_mutex_lock(&buffer->mutex);

        // sleep thread if buffer is empty, wait for full signal
        while(buffer->count == 0)
            pthread_cond_wait(&full, &buffer->mutex);

        line = buffer->input[*use_ptr];
        *use_ptr = (*use_ptr + 1) % BUFFERSIZE;
        buffer->count--;

        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&buffer->mutex);
        line = buffer->input[*use_ptr];
        fprintf(stdout,"%s\n", line);
        fflush(stdout);
    }

    fprintf(stderr, "\noutput thread done\n");

    return NULL;
}


//Input Thread(producer only): This thread collects input on a line-by-line basis from standard input.
void* getInput(void*args){
    char line[INPUTLINE_LENGTH+1] = "";
    Buffer * buffer = &buffer_array[0];
    size_t * fill_ptr = &buffer->fill_ptr;  //producer's index to buffer0
    // store line into buffer array (produce item)
    char * done = NULL;
    while(fgets(line, INPUTLINE_LENGTH+1, stdin) != NULL){
        pthread_mutex_lock(&buffer->mutex);
    
        // sleep producer if buffer is full, wait for empty signal
        while(buffer->count == BUFFERSIZE)
            pthread_cond_wait(&empty, &buffer->mutex);

        // produce lines by storing stdin into buffer
        if(strcmp(line, "DONE\n") == 0){
            strcpy(buffer->input[*fill_ptr], END_MARKER);
        }
        else{
            strcpy(buffer->input[*fill_ptr], line);
        }
        *fill_ptr = (*fill_ptr + 1) % BUFFERSIZE;
        buffer->count++;

        pthread_cond_signal(&full);
        pthread_mutex_unlock(&buffer->mutex);
    }
    fprintf(stderr, "\ngetInput done\n");
    return NULL;
}



int main(void){
    init();  // initialize mutexes and condition variables
    assert(pthread_cond_init(&full, NULL) == 0);
    assert(pthread_cond_init(&empty, NULL) == 0);
    pthread_t thread[4];
    pthread_create(&thread[0], NULL, getInput, NULL);
    pthread_create(&thread[1], NULL, lineSeparator, NULL);
    pthread_create(&thread[3], NULL, sendOut, NULL);
    
    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    pthread_join(thread[3], NULL);
    destroy();
    return 0;
}

// Thread function that converts the line separator into a space
// shares buffer01 with t1/getInput()
void* lineSeparator(void* args){
    // lineSeparator is a consumer for buffer 1
    Buffer * buffer = &buffer_array[0];
    size_t * use_ptr = &buffer->use_ptr;    //consumer's index to buffer0

    // lineSeparator is a producer for buffer 2
    Buffer * buffer2 = &buffer_array[1];
    size_t * fill_ptr = &buffer2->fill_ptr;

    char line[INPUTLINE_LENGTH+1] = "";
    while(strcmp(buffer->input[*use_ptr], END_MARKER) != 0){
        pthread_mutex_lock(&buffer->mutex);

        // sleep thread if buffer 1 is empty, wait for full signal from input thread
        while(buffer->count == 0)
            pthread_cond_wait(&full, &buffer->mutex);

        // consumer: get line from buffer 1, update consumer index and buffer 1 count
        strcpy(line,buffer->input[*use_ptr]);
        *use_ptr = (*use_ptr + 1) % BUFFERSIZE;
        buffer->count--;

        // signal empty to input thread, release lock to buffer 1
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&buffer->mutex);
        
        // producer: aquire lock for buffer 2
        pthread_mutex_lock(&buffer2->mutex);

        // when buffer 2 is full, wait for plus sign thread to signal empty
        while(buffer2->count == BUFFERSIZE)
            pthread_cond_wait(&empty, &buffer2->mutex);

        // producer: remove newline, copy line to buffer 2, update producer index and count
        line[strcspn(line, "\n")] = ' ';
        strcpy(buffer2->input[*fill_ptr], line);
        *fill_ptr = (*fill_ptr + 1) % BUFFERSIZE;
        buffer2->count--;

        // signal full to plus sign thread, release lock on buffer 2
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&buffer2->mutex);

    }
    fprintf(stderr, "\nlinesep return \n");

    // send END_MARKER to buffer2
    strcpy(buffer2->input[*fill_ptr],END_MARKER);
    
    return NULL;
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

int destroy() {
    int i ,r = 0;
    for(i = 0; i < NUM_BUFFERS; i++){
        r = pthread_mutex_destroy(&buffer_array[i].mutex);
    }
    pthread_cond_destroy(&full);
    return r;
}

int init(){
    int i, r = 0;
    // initialize mutexes, error handler doesn't quit program
    for(i = 0;i<NUM_BUFFERS; i++){
        r = pthread_mutex_init(&buffer_array[i].mutex, NULL);
        if(r != 0) {
            perror("mutex_init error"); //exit(0);
        }
    }

    return r;
}