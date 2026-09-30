#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h> 
#include <stdbool.h> 
#include <fcntl.h>
#include <ctype.h> 
#include <signal.h> 
#include <sys/times.h> 



#define BUFFER_SIZE 256

//a helpful struct to save in the array the word and the number of how many times the word was written
typedef struct {
    char word[BUFFER_SIZE];
    int countwords;
} SortedArray;

//function to compare the countwords of the struct
int CompareCountwords(const void *a, const void *b) {
    SortedArray *wordA = (SortedArray *)a;
    SortedArray *wordB = (SortedArray *)b;
    return wordB->countwords - wordA->countwords; 
}

//Global counter for USR1 signals and USR2 signals
int count_signal_splitters = 0;  
int count_signal_builders=0;

//functions for signal handling
void handle_usr1(int sig) {
    count_signal_splitters++;  //increment the counter when signal from splitter is received
}
void handle_usr2(int sig) {
    count_signal_builders++; //increment the counter when signal from builder is received
}

int main(int argc, char *argv[]) {
 
    //variables for calculating the time based on the given program in the project
    double t1 , t2 , cpu_time ;
    struct tms tb1 , tb2 ;
    double ticspersec ;
    ticspersec = ( double ) sysconf ( _SC_CLK_TCK );
    t1 = ( double ) times (& tb1) ;// start the clock

    // The signal handler for signal USR1
    if (signal(SIGUSR1, handle_usr1) == SIG_ERR) {
        perror("Unable to catch SIGUSR1");
        exit(1);
    }

    // The signal handler for signal USR2
    if (signal(SIGUSR2, handle_usr2) == SIG_ERR) {
        perror("Unable to catch SIGUSR2");
        exit(1);
    }

    //Variables to save the input parameters
    char inputFile[20];
    char exclusionList[20];
    char outputFile[20];
    int numOfSplitter,numOfBuilders,topPopular;
    
    // Check if the number of parameters are correct
    if( argc!= 13){
        printf("The parameters of the input command are wrong");
        exit(1);
    }

    // Save the parameters in variables
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 ) {
            strcpy(inputFile, argv[++i]); 
        }
        else if (strcmp(argv[i], "-l") == 0) {
            numOfSplitter = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-m") == 0 ) {
            numOfBuilders = atoi(argv[++i]); 
        }
        else if (strcmp(argv[i], "-t") == 0) {
            topPopular = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-e") == 0 ) {
            strcpy(exclusionList, argv[++i]); 
        }
        else if (strcmp(argv[i], "-o") == 0 ) {
            strcpy(outputFile, argv[++i]); 
        }
        else {
            printf("Unknown parameter: %s\n", argv[i]);
            exit(1); 
        }
    }

    //Open the input file for reading 
    int input_fd = open(inputFile, O_RDONLY);
    if (input_fd == -1) {
        perror("Error opening input file");
        return 1;
    }

    // Variables to store data while reading
    ssize_t bytes_read;
    int countline = 0;

    // Count the lines of the file so we can calculate how many lines will each splitter read 
    char ch;
    while ((bytes_read = read(input_fd, &ch, 1)) > 0) {
        if (ch == '\n') {
            countline++;
        }
    }

    //Close the input file
    close(input_fd);

    //open again the input file to map the first byte of every line 
    input_fd = open(inputFile, O_RDONLY);
    if (input_fd == -1) {
        perror("Error opening input file");
        return 1;
    }

    off_t offsets[countline];
    off_t byte_offset = 0;
    int line_number = 0;

    //count every byte and save to the array offsets the first byte of every line 
    offsets[line_number] = byte_offset;
    while (read(input_fd, &ch, 1) == 1) {
        byte_offset++;
        if (ch == '\n') {
            line_number++;
            if (line_number < countline) {
                offsets[line_number] = byte_offset;
            }
        }
    }

    //Close the input file
    close(input_fd);

    //create all the pipes we need for the program 
    pid_t pid;
    int pipe_splitters1[2]; //pipe between root and splitter for sending the number of lines every splitter will read
    if (pipe(pipe_splitters1) == -1) { 
        perror("pipe failed");
        exit(1);
    }
    int pipe_splitters2[2];//pipe between root and splitter for sending the number of the offset the splitter will start reading
    if (pipe(pipe_splitters2) == -1) { 
        perror("pipe failed");
        exit(1);
    }
    int pipe_builder_splitter[numOfBuilders][2];  // Pipes between the splitters and each builder for sending the words excluding the words from exclusion list 
    for (int i = 0; i < numOfBuilders; i++) {
        if (pipe(pipe_builder_splitter[i]) == -1) {
            perror("Pipe for builder-splitter communication failed");
            exit(1);
        }
    }
    int pipe_builders_root[numOfBuilders][2]; // Pipes between each builder and the root for sending the words with the frequency
    for (int i = 0; i < numOfBuilders; i++) {
        if (pipe(pipe_builders_root[i]) == -1) {
            perror("Pipe for builder-splitter communication failed");
            exit(1);
        }
    }

    
    // Create the splitters and execute the operations needed    
    for (int i = 0; i < numOfSplitter; i++) {

        //Create the process using forks
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
        
        // Splitter process
        if (pid == 0) {
            
            // Close the write end of the pipe in the splitter process
            close(pipe_splitters1[1]); 
            close(pipe_splitters2[1]);

            //close the read ends of the pipe between the splitter and builders that we will send through the arguments
            for (int i = 0; i < numOfBuilders; i++)
            {
                close(pipe_builder_splitter[i][0]);
            }
            
            // save into variables how many lines every splitter will read and the byte we will start to read 
            int received_lines_to_read ;
            read(pipe_splitters1[0], &received_lines_to_read, sizeof(received_lines_to_read));
            off_t offset_to_read;
            read(pipe_splitters2[0], &offset_to_read, sizeof(offset_to_read));
            
            //After use clode the read ends
            close(pipe_splitters1[0]);
            close(pipe_splitters2[0]); 
            

            // Dynamically allocate memory for the variables and pipes we will put in the arguments of the exec
            int total_args =6+ numOfBuilders + 1; 
            char **exec_args = malloc(total_args * sizeof(char *));

            //variables to save the converting data
            char lines_to_read_str[10];
            char offset_str[20];
            char numbuilder[10];
        
            // Use sprintf to convert the values into strings
            sprintf(lines_to_read_str, "%d", received_lines_to_read);//array
            sprintf(offset_str, "%ld", offset_to_read);  //offsets[line[i]]
            sprintf(numbuilder, "%d",numOfBuilders);

            exec_args[0] = "./splitter";
            exec_args[1]= inputFile;
            exec_args[2] =exclusionList;
            exec_args[3]= lines_to_read_str;
            exec_args[4] =offset_str;
            exec_args[5] =numbuilder;

            //convert every pipe into a string so we can use it as argument in the execvp
            int arg_idx = 6;
            for (int j = 0; j < numOfBuilders; j++) {
                char *write_fd_str = malloc(10 * sizeof(char));
                sprintf(write_fd_str, "%d", pipe_builder_splitter[j][1]);

                exec_args[arg_idx++] = write_fd_str;//save in the array
            }  
            exec_args[arg_idx]=NULL; // the last argument is null   

            execvp(exec_args[0], exec_args); //the exec replaces this line in the process with a new program
            perror("exevp failed");// if the program will continue after the exec then the exec failed
            exit(0); 
        }
    }
    

    //Create the builders and execute the operations needed 
    for (int i = 0; i < numOfBuilders; i++) {

        //Create the process using forks
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        }
        
        // builder process
        if (pid == 0) {

            // Close the ends the builder will not use 
            for (int j = 0; j < numOfBuilders; j++) {
                close(pipe_builder_splitter[j][1]);  // Close write ends of the pipe between splitter and builder because in the builder we will only read  
                close(pipe_builders_root[j][0]);  //close read ends of the pipe between root and builder because in the builder we will only write the results  
            }

            //variables to save the converting data so we will use as arguments in the exevp
            char read_fd_str[10];
            char write_fd_str[10];
            char countile_fd[20];

            // Use sprintf to convert the values into strings
            sprintf(read_fd_str, "%d", pipe_builder_splitter[i][0]); 
            sprintf(write_fd_str, "%d", pipe_builders_root[i][1]); 
            sprintf(countile_fd, "%d", countline);  


            char *exec_args[] = {
                "./builder",
                read_fd_str, 
                countile_fd,
                write_fd_str,
                NULL         
            };


            execvp(exec_args[0], exec_args);//the exec replaces this line in the process with a new program
            perror("exevp failed");// if the program will continue after the exec then the exec failed
            exit(0);
        }
    }

   
   //We check if we are in the root process
   if (pid > 0) {
        
        // Close the read end of the pipe in the root process 
        close(pipe_splitters1[0]); 
        close(pipe_splitters2[0]); 

        //Calculate how many lines will each splitter have and save in the array  so we can tranfer the data
        int add = 0;  //variable to track how many additional lines we are going to add
        int array[numOfSplitter];  
        for (int i = 0; i < numOfSplitter; i++)
        {
            int calculating_lines = countline / numOfSplitter;

            //check if the total number of lines cannot be divided exactly for each splitter
            if (countline % numOfSplitter != 0) {
                add++; 

                if (add <= countline % numOfSplitter) {
                    // Add to each splitter a line until we have completed the remaing lines
                    array[i] = calculating_lines + 1;
                } else {
                    // If we have completed the lines that remained assign the calculated number 
                    array[i] = calculating_lines;
                }
            } else {
                // If the lines are divided exactly assign the calculated number to each splitter
                array[i] = calculating_lines;
            }
        }
        
        int line;
        for (int i = 0; i < numOfSplitter; i++)
        {   
            line=array[i]*i;

            write(pipe_splitters1[1], &array[i], sizeof(array[i]));
            write(pipe_splitters2[1], &offsets[line], sizeof(offsets[line])); 
        }

        //close the pipes after writing the needed data 
        close(pipe_splitters1[1]); 
        close(pipe_splitters2[1]); 
        
        //Close the pipes between the builder and splitter after using it 
        //and the write end of the pipe between the builder and root 
        for (int i = 0; i < numOfBuilders; i++) {
            close(pipe_builder_splitter[i][1]);  
            close(pipe_builder_splitter[i][0]); 
            close(pipe_builders_root[i][1]);  
        }

        // Parent waits for each child process to terminate
        for (int i = 0; i < (numOfSplitter+numOfBuilders); i++) {
            wait(NULL); 
        }

        // Open the output file for writing (create if not exists)
        int output_fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC);
        if (output_fd == -1) {
            perror("Error opening output file");
            close(input_fd);
            return 0;
        }
        dprintf(output_fd,"This are the times for each builder\n");

    
        int wordCount = 0;   // Number of words in the array
        int arrayCapacity = 100;     //Initial capacity for the array


        // Allocate initial memory for the array
        SortedArray *sortedarray = malloc(arrayCapacity * sizeof(SortedArray));
        int intresting_words=0; //a variable to count the total number of intresting words so we can calculte the frequency
    
        //for each builder save the words and the frequency 
        for (int i = 0; i < numOfBuilders; i++) {
            
            int word_length;
            char word[BUFFER_SIZE];
            int countwords;

            // Read data from each builder until a termination marker is encountered
            while (read(pipe_builders_root[i][0], &word_length, sizeof(word_length)) > 0) {
                
                //if the length is 0 then we are done reading the data from the pipe
                //the 0 is the termination marker send from the builder 
                if (word_length == 0) {
                    break;
                }

                if (read(pipe_builders_root[i][0], word, word_length) != word_length) {
                    perror("read word from pipe failed");
                    break;
                }
                word[word_length] = '\0'; 

                //Read the number of the word
                read(pipe_builders_root[i][0], &countwords, sizeof(countwords));

                //if the array is full we need to reallocate more memory
                if (wordCount == arrayCapacity) {
                    arrayCapacity =arrayCapacity* 2;
                    SortedArray *temp = realloc(sortedarray, arrayCapacity * sizeof(SortedArray));
                    if (temp == NULL) {
                        perror("Failed to reallocate memory");
                        free(sortedarray);
                        exit(1);
                    }
                    sortedarray = temp;
                }

                // Add the word and countwords to the array
                strcpy(sortedarray[wordCount].word, word);
                sortedarray[wordCount].countwords = countwords;
                
                //add every time variable countwords which has saved how many times the word was writen
                intresting_words+=countwords;

                //next word 
                wordCount++;
            
            }

            // After the termination marke read the timing information of each builder
            double real_time, cpu_time;
            read(pipe_builders_root[i][0], &real_time, sizeof(real_time));
            read(pipe_builders_root[i][0], &cpu_time, sizeof(cpu_time));
            dprintf(output_fd,"Builder %d: Real time = %lf sec, CPU time = %lf sec\n", i + 1, real_time, cpu_time);

        }

        //sort the array by the countwords
        qsort(sortedarray, wordCount, sizeof(SortedArray), CompareCountwords);

        // Print the top popular words 
        dprintf(output_fd, "\nThe top %d popular words in the file are :\n",topPopular);
        for (int i = 0; i < topPopular; i++) {
            dprintf(output_fd, "Word: %s, Frequency: %.2f %% \n", sortedarray[i].word, (float)sortedarray[i].countwords / intresting_words * 100);
        }


        // free the allocated array 
        free(sortedarray);

        //Close all the pipes after using 
        for (int i = 0; i < numOfBuilders; i++) {
            close(pipe_builders_root[i][1]);  
            close(pipe_builders_root[i][0]); 
        }

        // Print the signals in the outputfile
        dprintf(output_fd,"\nNumber of SIGUSR1 signals received from splitter: %d\n", count_signal_splitters);
        dprintf(output_fd,"Number of SIGUSR2 signals received from builder: %d\n\n", count_signal_builders);
         
        // print the time needed to terminate the program
        t2 = ( double ) times (& tb2) ;
        cpu_time = ( double ) (( tb2 . tms_utime + tb2 . tms_stime ) -( tb1 . tms_utime + tb1 . tms_stime ));
        dprintf (output_fd,"Run time of the main program was %lf sec and we used the CPU for %lf sec \n", (t2 - t1) / ticspersec , cpu_time / ticspersec );

        //close the output file
        close(output_fd);
    }
   
    
    return 0;
}