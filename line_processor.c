#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#define INPUTLINE_LENGTH 1000
#define OUTPUTLINE_LENGTH 80



int main(void){

    char input[OUTPUTLINE_LENGTH+1] = {'\0'};
    int n = OUTPUTLINE_LENGTH;
    while(fgets(input, n, stdin) != NULL){

        // print partial line (lines that have less than 80 chars)
        if(strchr(input, '\n') != NULL){    //partial lines contain a newline char
            input[strcspn(input, "\n")] = ' ';  // replace newline with space
            printf("%s",input);
            n -= strlen(input);     //offset the next fgets by n - length of partial line
            continue;
        }
        n = OUTPUTLINE_LENGTH;
        puts(input);
    }





    return 0;
}
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++79
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80
// s, which generates green rust, is an example.