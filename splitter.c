#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 
#include <ctype.h> 
#include <signal.h>  

#define BUFFER_SIZE 256

#include "Hash.h"


// Function that removes punctuation marks from a word( for example: , . [] " )
void remove_punctuation(char *word) ;

//function that tranfirms the capital letters into small ones
void transform_capitals(char *str);

// Check if the word is an integer
bool is_integer(const char *word);

//a hash function to calculate in which builder will the word go
int hash_for_builders(const char *str, int numbuilders);

int main (int argc, char *argv[]){
    
    //variables for reading and storing the words 
    ssize_t bytes_read; // stores how many bytes will the loop read each time
    char line[BUFFER_SIZE]; //stores the word in every line
    int line_pos = 0; //helpful variable for storing the word into the array line[Buffer_size]
    char buffer[BUFFER_SIZE]; // stores all the characters the loop read

    //variable for counting the lines in the exclusion list
    int countline_execlusion = 0;

    //open the exclusion list
    int fd_exclusionlist = open(argv[2], O_RDONLY);
    if (fd_exclusionlist == -1) {
        perror("Error opening file");
        exit(1);
    }
    
    // Count the lines of the file
    char ch;
    while ((bytes_read = read(fd_exclusionlist, &ch, 1)) > 0) {
        if (ch == '\n') {
            countline_execlusion++;
        }
    }
    
    //go to the begging of the file so we can read the data
    lseek(fd_exclusionlist, 0, SEEK_SET);

    //create the hash table  that we will use for saving the words in the exclusion list  
    HashTable exclusion_table = create_hash_table(countline_execlusion*countline_execlusion);

    //we read parts of data and store it into the buffer . In the variable bytes_read it will be saves how many bytes we read
    //the read will continue until we read all the data from the file 
    while ((bytes_read = read(fd_exclusionlist, buffer, sizeof(buffer))) > 0) {
        
        for (int i = 0; i < bytes_read; i++) {
            //for each byte we will check if the current character is \n 
            if (buffer[i] == '\n') {
                
                line[line_pos] = '\0';//last  character of the word
                hash_add(exclusion_table, strdup(line));//add the word into the table using strdup which will duplicate the word 
                line_pos = 0;// initialize the line position with 0 for the next word 

            } else {
                // If the character is not a \n then we will add the character into the word 
                line[line_pos++] = buffer[i];
            }
        }
    }
   
    //close the file exclusion list 
    close(fd_exclusionlist);

    //cast and store variables from the arguments
    int lineread = atoi(argv[3]);//how many lines will this splitter read
    off_t offset=atoi(argv[4]); //the first byte of the line the splitter will start to read
    int numOfBuilders = atoi(argv[5]); // the number of builders

    //allocate memory for the pipes between the splitters and the builders
    int *pipe_write_fds = malloc(numOfBuilders * sizeof(int));
    if (!pipe_write_fds) {
        perror("malloc failed");
        exit(1);
    }

    //cast the pipes from the arguments
    for (int i = 0; i < numOfBuilders; i++) {
        pipe_write_fds[i] = atoi(argv[6 + i]);
    }
    

    line_pos = 0; //helpful variable that shows the position that the character will be saved in the array
    int countline = 0; //counting the lines the splitter will read so we can know when to stop reading
    int flag = 0; //a flag that helps me exit the loops

    //open the file we will be reading data from
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    //based on the offset go the line we want to start reading
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking file");
        close(fd);
        exit(1);
    }

    //begin the reading process we by saving in the variable bytes_read how many bytes we will read in each loop
    //the read will continue until we read all the data from the file 
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        
        for (int i = 0; i < bytes_read; i++) {
            // we stop the reading if the desired number of lines has been  reached
            if (countline >= lineread) {
                flag = 1; //make the flag true so we can exit the while loop
                break;
            }
            //the word stops when we have newline or spacetab
            if (buffer[i] == '\n' || buffer[i] == ' ') {       
                
                line[line_pos] = '\0'; //the last character of the word       
                remove_punctuation(line);//remove the punctuation from every word
                transform_capitals(line);//tranform every capital letter to small
                if (strlen(line) > 0 && !is_integer(line)) {  // Check if the word is not an integer
                    
                    if (hash_find(exclusion_table, line) == NULL) {//check if the word belongs to the exclusion list with O(1) complexity
                        
                        //calculate the index of the builser we will send the data so we will send the same words in the same splitters 
                        int index_builder= hash_for_builders(line,numOfBuilders);

                        //add a space after every word so we can separate them
                        strcat(line, " ");

                        //write to the corresponding pipe
                        if (write(pipe_write_fds[index_builder], line, strlen(line)) == -1) {
                            perror("write to pipe failed");
                            free(pipe_write_fds);
                            return 1;
                        }

                    }
                
                }

                line_pos = 0;// initialize the line position with 0 for the next word 
                if(buffer[i] == '\n'){ // increase the countline if we change the line 
                    countline++; 
                }
                
            } else {
                // If the character is not a \n or a spacetab then we will add the character into the word 
                line[line_pos++] = buffer[i];
            }
           
        }
        if (flag) {
            break;  // break the outer loop if the flag is set
        }
    }

    //close the file 
    close(fd);
    
    //delete the hash table 
    delete_hash_table(exclusion_table);

    //send a signal when the splitter ends his process
    if ( kill(getppid(), SIGUSR1)) {
        perror("Failed to send SIGUSR1 to parent");
    }
    
    exit(0);
}

//The hash function for which builder each word will be tranfered 
int hash_for_builders(const char *str, int numbuilders){

    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  
    }
    return hash % numbuilders;
}


void remove_punctuation(char *word) {
    int i = 0, j = 0;
    while (word[i] != '\0') {
        //isalpha separates the alphabetic characters from the punctuation characters
        if (isalpha(word[i])) { 
            word[j++] = word[i];
        }
        i++;
    }
    word[j] = '\0';  
}

//return if the word is an integer or not
bool is_integer(const char *word) {

    for (int i = 0; word[i] != '\0'; i++) {
        if (!isdigit(word[i])) {
            return false; 
        }
    }
    return true; 
}

void transform_capitals(char *str) {
    int i = 0;
    //each character of the word will be converted from capitals into small letters
    while (str[i] != '\0') {
        str[i] = tolower(str[i]); 
        i++;
    }
}