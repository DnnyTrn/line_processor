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

typedef struct
{
    char input[BUFFERSIZE][INPUTLINE_LENGTH + 1];
    pthread_mutex_t mutex;
    size_t count, fill_ptr, use_ptr;
} Buffer;

Buffer buffer_array[NUM_BUFFERS] = { 0 };

pthread_cond_t full, empty;

int init();
int destroyMutex();
void *lineSeparator(void *args);
void *getInput(void *args);
void *sendOut(void *arg);
void *plusSignRemove(void *args);
int producerLock(Buffer *buffer);
int producerUnlock(Buffer *buffer);
void checkExitWord(Buffer * buffer, const char* line, const char * exitWord, int*work);
int _incrementProducerIndex(Buffer * buffer);
int producerPutLine(Buffer * buffer, char * line);
int consumerGetLine(Buffer * buffer, char * line);
int _plusSignRemove(char * line);

// Consumer thread that sends 80 chars to standard out
void *sendOut(void *arg) {
    Buffer *bufferConsumer = &buffer_array[2];

    char line[INPUTLINE_LENGTH + 1] = "";
    char outputline[OUTPUTLINE_LENGTH + 1] = "";
    size_t n;   //offset for outputline
    int work = 1;
    while (work) {
        consumerGetLine(bufferConsumer, line); // aquire line from buffer 3
        checkExitWord(NULL, line, END_MARKER, &work); //check if line has exit word
        if(work == 1){
            n = strlen(outputline);     // n represents number of characters from the last line
            size_t offset = 0, line_length = strlen(line);
            while(offset < line_length){
                strncpy(outputline+n, line+offset, OUTPUTLINE_LENGTH-n);
                n = strlen(outputline);
                offset += n;
                if(n == OUTPUTLINE_LENGTH){
                    fprintf(stdout, "%s\n", outputline); 
                    memset(outputline, 0, sizeof(outputline));
                    n = 0;  //reset offset for outputline
                }
                printf("%zu\n", strlen(outputline));
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void *plusSignRemove(void *arg)
{
    Buffer *bufferConsumer = &buffer_array[1];    //thread is consumer for buffer 2
    Buffer *bufferProducer = &buffer_array[2];    // thread is producer for buffer 3

    char line[INPUTLINE_LENGTH + 1] = "";
    int work = 1;
    while (work)
    {
        consumerGetLine(bufferConsumer, line);
        producerLock(bufferProducer);
        checkExitWord(bufferProducer, line, END_MARKER, &work);        // check if line is end marker
        if (work == 1) {
            _plusSignRemove(line);     //producer work: replace every instance of ++ with ^
            producerPutLine(bufferProducer, line);     // put formatted line onto buffer 3, increment fill_ptr and buffer 3 count
        }
        producerUnlock(bufferProducer); // signal to waiting threads that buffer 3 has lines, release mutex lock on buffer 3
    }
    return NULL;
}

int main(void)
{
    init(); // initialize mutexes and condition variables

    pthread_t thread[4];
    pthread_create(&thread[0], NULL, getInput, NULL);
    pthread_create(&thread[1], NULL, lineSeparator, NULL);
    pthread_create(&thread[2], NULL, plusSignRemove, NULL);
    pthread_create(&thread[3], NULL, sendOut, NULL);

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);
    pthread_join(thread[2], NULL);
    pthread_join(thread[3], NULL);
    destroyMutex();
    return 0;
}

//Input Thread(producer only): This thread collects input on a line-by-line basis from standard input.
void *getInput(void *args)
{
    Buffer *buffer = &buffer_array[0];
    char line[INPUTLINE_LENGTH + 1] = "";
    int work = 1;
    while (work)
    {
        fgets(line, INPUTLINE_LENGTH + 1, stdin);

        producerLock(buffer);  // sleep producer if buffer is full, wait for empty signal

        checkExitWord(buffer, line, "DONE\n", &work);
        if (work == 1) {
            producerPutLine(buffer, line);// produce lines by storing stdin into buffer
        }
        producerUnlock(buffer);
    }
    return NULL;
}
// Thread function that converts the line separator into a space
void *lineSeparator(void *args)
{
    Buffer *bufferConsumer = &buffer_array[0];    // lineSeparator is a consumer for buffer 1
    Buffer *bufferProducer = &buffer_array[1];    // lineSeparator is a producer for buffer 2

    char line[INPUTLINE_LENGTH + 1] = "";
    int work = 1;
    while (work)
    {
        consumerGetLine(bufferConsumer, line);  //aquire line from buffer 1 
        producerLock(bufferProducer);   // producer: aquire lock for buffer 2
        checkExitWord(bufferProducer, line, END_MARKER, &work);  // check if line is end marker..

        if (work == 1) {
            line[strcspn(line, "\n")] = ' ';    // producer work: remove newline from line
            producerPutLine(bufferProducer, line);  //put worked on line onto buffer 2
        }
        producerUnlock(bufferProducer); // signal full to plus sign thread, release lock on buffer 2
    }
    return NULL;
}

int destroyMutex()
{
    int i, r = 0;
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        r = pthread_mutex_destroy(&buffer_array[i].mutex);
    }
    pthread_cond_destroy(&full);
    return r;
}
int init()
{
    int i, r = 0;
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        r = pthread_mutex_init(&buffer_array[i].mutex, NULL);    // initialize mutexes, error handler doesn't quit program
        if (r != 0)
        {
            perror("mutex_init error"); //exit(0);
        }
    }

    r = pthread_cond_init(&full, NULL);    // ERROR handler for pthread_cond_init without exit()
    if (r != 0)
        perror("pthread_cond_init error");
    r = pthread_cond_init(&empty, NULL);
    if (r != 0)
        perror("pthread_cond_init error");
    return r;
}

int producerLock(Buffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);
    while (buffer->count == BUFFERSIZE)    // sleep producer if buffer is full, wait for empty signal from consumerGetLine()
        pthread_cond_wait(&empty, &buffer->mutex);
    return 0;
}

int producerUnlock(Buffer *buffer) {
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
}
int _incrementProducerIndex(Buffer * buffer) {
    size_t *fill_ptr = &buffer->fill_ptr; //producer's index to buffer
    *fill_ptr = (*fill_ptr + 1) % BUFFERSIZE;
    buffer->count++;
    return 0;
}
void checkExitWord(Buffer * buffer, const char* line, const char * exitWord, int * work) { 
    if (strcmp(exitWord, line) == 0)
    {
        if(buffer != NULL)
            producerPutLine(buffer, END_MARKER); // put end marker onto buffer as a producer to signal consumer when to stop working
        *work = 0; //exit while loop of current thread to stop working and return to main
    }
}

int consumerGetLine(Buffer * buffer, char * line) {
    pthread_mutex_lock(&buffer->mutex); //consumer aquire lock on buffer
    while (buffer->count == 0)
        pthread_cond_wait(&full, &buffer->mutex);    // sleep consumer if buffer is empty, wait for full signal from producer thread

    size_t *use_ptr = &buffer->use_ptr; //consumer pointer to buffer
    strcpy(line, buffer->input[*use_ptr]);  // consumer: get line from buffer , increment consumer index and decrement buffer count
    *use_ptr = (*use_ptr + 1) % BUFFERSIZE;
    buffer->count--;

    pthread_cond_signal(&empty);    // signal empty to waiting producer threads (producerLock() function), release lock to buffer 
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
}

int producerPutLine(Buffer * buffer, char * line) {
    strcpy(buffer->input[buffer->fill_ptr], line);
    _incrementProducerIndex(buffer);
    return 0;
};

int _plusSignRemove(char * line) {
    // line is not end marker, process line.
    //producer: replace every instance of ++ with ^
    size_t len = strlen(line), i, j;
    for (i = 0; i < len; i++) {
        if (line[i] == '+' && line[i + 1] == '+')
        {
            line[i] = '^';
            for (j = i + 1; j < len - 1; j++) {
                line[j] = line[j + 1];  // shift each letter left by 1
            }
            line[len - 1] = '\0';   // erase last letter since it's a duplicate
        }
    }
    return 0;
}