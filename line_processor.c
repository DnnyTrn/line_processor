#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#define INPUTLINE_LENGTH 1000
#define OUTPUTLINE_LENGTH 80



int main(void){

    char input[OUTPUTLINE_LENGTH+1] = "", 
        beginning[OUTPUTLINE_LENGTH+1] = "";

    int n = OUTPUTLINE_LENGTH;
    while(fgets(input, n+1, stdin) != NULL){
    
        // terminate program on DONE\n
       char * done = strstr(input, "DONE\n");
        if(done && strlen(input) == 5) break;
    
        // print partial line if there is one
        if(strlen(beginning) > 0){
            printf("%s", beginning);
            memset(beginning, 0, sizeof(beginning));
        }

        // check for partial line (lines that have less than 80 chars)
        if(strchr(input, '\n') != NULL){    //partial lines contain a newline char
            input[strcspn(input, "\n")] = ' ';  // replace newline with space
            strcpy(beginning, input);   //partial line will be printed in next iteration of fgets 
            n -= strlen(input);     //offset the next fgets by n - length of partial line
            continue;
        }

        n = OUTPUTLINE_LENGTH;  //number of chars to fgets from stdin
        
        // process line before printing
        puts(input);
    }





    return 0;
}
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++79
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80
// Rust is an iron oxide, a usually reddish brown oxide formed by the ++reaction++ 80
// s, which generates green rust, is an example.