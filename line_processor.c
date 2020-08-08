#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define INPUTLINE_LENGTH 1000
#define OUTPUTLINE_LENGTH 80

int main(void){

    char input[OUTPUTLINE_LENGTH+1];

    // size_t size = 0;
    // int nread;
    // int nread_total = 0;
    fgets(input, OUTPUTLINE_LENGTH+1, stdin);
    // nread_total += nread;

    fprintf(stdout, input, OUTPUTLINE_LENGTH+1);

    // printf("sizeof %zu size %zu nread %d\n", sizeof(input), size, nread);
    // free(input);
    return 0;
}
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++79
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80