#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include <signal.h>
#include <sys/times.h>


#include "Hash.h"
#define BUFFER_SIZE 256


int main(int argc, char *argv[]){

    //variables for calculating the time based on the given program in the project
    double t1 , t2 , cpu_time ;
    struct tms tb1 , tb2 ;
    double ticspersec ;
    ticspersec = ( double ) sysconf ( _SC_CLK_TCK );
    t1 = ( double ) times (& tb1) ;// start the clock

    //cast and store how many lines has the file so we can create the hash table 
    int countline = atoi(argv[2]);
  
    //cast the pipes from the arguments
    int pipe_read_fd= atoi(argv[1]); // this is the pipe we will be reading the words from the splitter
    int pipe_write_fd= atoi(argv[3]);// this is the pipe we will be writing the words and the frequency to the root 
    
    char buffer[BUFFER_SIZE]; // Buffer for storing data the readind data
    ssize_t bytesRead;  // stores how many bytes will the loop read each time
    HashTable mytable=create_hash_table(countline*2); // create the table we will be storing the words

    
    //begin the reading process  from the pipes and save in the bytes_read how many bytes we will read in each loop
    //the read will continue until we read all the words from the pipe 
    while ((bytesRead = read(pipe_read_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; 

        int start = 0; //initialize the index of the current word
        for (int i = 0; i <= bytesRead; i++) {
            
            if (buffer[i] == ' ' || buffer[i] == '\0') {
                if (i > start) { // make sure that there is a word to print
                    char word[i - start + 1]; // Temporary storage for the word
                    strncpy(word, &buffer[start], i - start);
                    word[i - start] = '\0';

                    //add the word to the hash table using strdup that duplicates the word
                    //if we add the same word the hash table will increase the frequency
                    hash_add(mytable,strdup(word));
                    
                }
                start = i + 1; // Move to the start of the next word
            }
        }
    }

    //iterate through all the words in the hash table and write them in the pipe 
    HashNode node = hash_first(mytable);
    while (node != NULL) {

        char *value = hash_get_value(mytable, node); //get the value of the word 
        int frequency_number=get_counter(mytable,value); //and get the frequency of the word

        int length = strlen(value);
        //write how many bytes are excected the root to read
        if (write(pipe_write_fd, &length, sizeof(length)) == -1) {
            perror("write length to pipe failed");
        }
        //write the word
        if (write(pipe_write_fd, value, strlen(value)) == -1) {
            perror("write word to pipe failed");
        }
        //write the frequency number
        if (write(pipe_write_fd, &frequency_number, sizeof(frequency_number)) == -1) {
            perror("write frequency to pipe failed");
        }
        node = hash_next(mytable, node);// go to the next word
    }
    
    // this variable will show when the writing in the pipe will be terminated
    int termination_marker = 0;
    if (write(pipe_write_fd, &termination_marker, sizeof(termination_marker)) == -1) {
        perror("write termination marker failed");
    }

    // End timing
    t2 = times(&tb2);
    t2 = ( double ) times (& tb2) ;
    cpu_time = ( double ) (( tb2 . tms_utime + tb2 . tms_stime ) -( tb1 . tms_utime + tb1 . tms_stime ));
    double real_time = (t2 - t1) / ticspersec;
    cpu_time =cpu_time / ticspersec;

    // Write the timing information to the pipe
    if (write(pipe_write_fd, &real_time, sizeof(real_time)) == -1) {
        perror("write real time to pipe failed");
    }
    if (write(pipe_write_fd, &cpu_time, sizeof(cpu_time)) == -1) {
        perror("write CPU time to pipe failed");
    }

    //send a signal when the builder ends his process
    if (kill(getppid(), SIGUSR2) == -1) {
        perror("Failed to send SIGUSR2 to parent");
    }

    exit(0);
}