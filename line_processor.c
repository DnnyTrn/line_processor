//Author: Danny Tran
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
    size_t count, fill_ptr, use_ptr;
    pthread_mutex_t mutex;
    pthread_cond_t full, empty;
} Buffer;

Buffer buffer_array[NUM_BUFFERS] = {0};

int init();
int destroy();
void *lineSeparator(void *args);
void *getInput(void *args);
void *sendOut(void *arg);
void *plusSignRemove(void *args);
int _plusSignRemove(char *line);
void checkExitWord( Buffer *const buffer, char *const line,  char *const exitWord, int *const work);
int producerPutLine(Buffer *const buffer,  char *const line);
int consumerGetLine(Buffer *const buffer,  char *const line);

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
    destroy();
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
        checkExitWord(buffer, line, "DONE\n", &work);
        if (work == 1)
        {
            producerPutLine(buffer, line); // produce lines by storing stdin into buffer
        }
    }
    return NULL;
}
// Thread function that converts the line separator into a space
void *lineSeparator(void *args)
{
    Buffer *bufferConsumer = &buffer_array[0]; // lineSeparator is a consumer for buffer 1
    Buffer *bufferProducer = &buffer_array[1]; // lineSeparator is a producer for buffer 2

    char line[INPUTLINE_LENGTH + 1] = "";
    int work = 1;
    while (work)
    {
        consumerGetLine(bufferConsumer, line);                  //aquire line from buffer 1
        checkExitWord(bufferProducer, line, END_MARKER, &work); // check if line is end marker..
        if (work == 1)
        {
            line[strcspn(line, "\n")] = ' ';       // producer work: remove newline from line
            producerPutLine(bufferProducer, line); //put worked on line onto buffer 2
        }
    }
    return NULL;
}
// Consumer and Producer thread that works between buffer 2 and 3 to replace every instance of ++ with ^
void *plusSignRemove(void *arg)
{
    Buffer *bufferConsumer = &buffer_array[1]; //thread is consumer for buffer 2
    Buffer *bufferProducer = &buffer_array[2]; // thread is producer for buffer 3

    char line[INPUTLINE_LENGTH + 1] = "";
    int work = 1;
    while (work)
    {
        consumerGetLine(bufferConsumer, line);  //grab line from buffer 1 (threadsafe)
        checkExitWord(bufferProducer, line, END_MARKER, &work); // check if line is end marker
        if (work == 1)
        {
            _plusSignRemove(line); //producer work: replace every instance of ++ with ^
            producerPutLine(bufferProducer, line); // put formatted line onto buffer 3, increment fill_ptr and buffer 3 count
        }
    }
    return NULL;
}

// Consumer thread that sends 80 chars to standard out
void *sendOut(void *arg)
{
    Buffer *bufferConsumer = &buffer_array[2];

    char line[INPUTLINE_LENGTH + 1] = "";
    char outputline[OUTPUTLINE_LENGTH + 1] = "";
    size_t x = 0; //offset for outputline
    int work = 1;
    while (work)
    {
        consumerGetLine(bufferConsumer, line);  // safely aquire line from buffer 3
        checkExitWord(NULL, line, END_MARKER, &work); //check if line has exit word, pass NULL for buffer since there is no thread to signal after this one.
        if (work == 1) //threadsafe work, outputline is being created locally
        {
            size_t i, len = strlen(line);
            for (i = 0; i < len; i++)
            {
                outputline[x++] = line[i];
                if (x >= OUTPUTLINE_LENGTH)
                {          //print output line for every 80 characters
                    x = 0; //reset output index
                    fprintf(stdout, "%s\n", outputline);
                    memset(outputline, 0, sizeof(outputline));
                }
            }
            fflush(stdout);
        }
    }
    return NULL;
}

// destroy all mutexes and condvars in buffer_array
// Precondition: struct Buffer buffer_array[NUM_BUFFERS] exists in global scope
int destroy()
{
    int i, r = 0;
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        r = pthread_mutex_destroy(&buffer_array[i].mutex);
        r = pthread_cond_destroy(&buffer_array[i].empty);
        r = pthread_cond_destroy(&buffer_array[i].full);
    }
    return r;
}

// initialize all mutex in buffer_array and condition variables, run before calling threads
// Precondition: struct Buffer buffer_array[NUM_BUFFERS] exists in global scope
int init()
{
    int i, r = 0;
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        r = pthread_mutex_init(&buffer_array[i].mutex, NULL); // initialize mutexes, error handler doesn't quit program
        if (r != 0)
        {
            perror("mutex_init error"); //exit(0);
        }
        r = pthread_cond_init(&buffer_array[i].full, NULL); // ERROR handler for pthread_cond_init without exit()
        if (r != 0)
            perror("pthread_cond_init error");
        r = pthread_cond_init(&buffer_array[i].empty, NULL);
        if (r != 0)
            perror("pthread_cond_init error");
    }
    return r;
}

void checkExitWord( Buffer *const buffer, char *const line,  char *const exitWord, int *const work)
{
    if (strcmp(exitWord, line) == 0) //check if line is the exit word.
    {
        if (buffer != NULL) //for producer threads only.
            producerPutLine(buffer, END_MARKER); // put end marker onto buffer as a producer to signal consumer when to stop working
        *work = 0; //exit while loop of current thread to stop working and return to main
    }
}

// Thread-safe function to aquire line from buffer
int consumerGetLine(Buffer *const buffer, char *const line)
{
    pthread_mutex_lock(&buffer->mutex); //consumer aquire lock on buffer
    while (buffer->count == 0)
        pthread_cond_wait(&buffer->full, &buffer->mutex); // sleep consumer if buffer is empty, wait for full signal from producer thread

    strcpy(line, buffer->input[buffer->use_ptr]); // consumer: get line from buffer
    buffer->use_ptr = (buffer->use_ptr + 1) % BUFFERSIZE; // point to the next index on buffer for consumer to grab next (wraps around to 0)
    buffer->count--; // decrement buffer count

    pthread_cond_signal(&buffer->empty); // signal empty to waiting producer threads (producerLock() function), release lock to buffer
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
}

// Thread-safe function to add line onto bufferconst 
int producerPutLine(Buffer *const buffer, char *const line)
{
    pthread_mutex_lock(&buffer->mutex);
    while (buffer->count == BUFFERSIZE) // sleep producer if buffer is full, wait for empty signal from consumerGetLine()
        pthread_cond_wait(&buffer->empty, &buffer->mutex);

    strcpy(buffer->input[buffer->fill_ptr], line); //put line onto buffer
    buffer->fill_ptr = (buffer->fill_ptr + 1) % BUFFERSIZE; //point to next index on buffer for producer  (wraps around to 0)
    buffer->count++; // increase buffer count

    pthread_cond_signal(&buffer->full);
    pthread_mutex_unlock(&buffer->mutex);
    return 0;
};

// helper function for plusSignRemove() thread
int _plusSignRemove(char * line)
{
    // line is not end marker, process line.
    //producer: replace every instance of ++ with ^
    size_t len = strlen(line), i, j;
    for (i = 0; i < len; i++)
    {
        if (line[i] == '+' && line[i + 1] == '+')
        {
            line[i] = '^';
            for (j = i + 1; j < len - 1; j++)
            {
                line[j] = line[j + 1]; // shift each letter left by 1
            }
            line[len - 1] = '\0'; // erase last letter since it's a duplicate
        }
    }
    return 0;
}
