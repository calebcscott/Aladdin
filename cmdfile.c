/************************* cmdfile *******************************
 * Purpose:
 *  Library code to aid in reading commands dynamically from a file
 *      into memory as well as writing new commands into file
 * 
 *  Authors:
 *      calebcscott - calebcscott@outlook.com
 *      SamBkamp
 * 
 *  Notes:
 *    Public prototypes:
 *      read_commands - used to load commands from file option to pass
 *                          file_name otherwise default "commands.csv" 
 * 
 *      add_command - allows caller to add a command dynamically to 
 *                          command list, new command will be added without
 *                          having to re-read disk
 * 
 *      test_command - used to find matching command in list and returns
 *                          message
 * 
 *      close_commnads - used to write new commands into file and free
 *                          memory of loaded commands
 *    Private prototypes:
 *      find_command - used to find command in list returns Commands struct pointer
 * 
 *      write_commands - used to append to command file any new commands in list if flag set
 * 
 *    Other:
 *      File Structure for csv or text file used should be as follows
 *        with no whitespace around comma (message may contain whitespace between words):
 *          <command>,<message> 
 * 
***************************************************************/


/*
    Adding debug output defined from user Johnathan Leffler at 
        stackoverflow.com/questions1644868/define-macro-for-debug-printing-in-c

    * should be moved to commone Debug header file for rest of project *
*/
#define debug_print(fmt, ...) \
    do { fprintf(stderr, "DEBUG:%s:%d:%s(): " fmt, __FILE__, \
                            __LINE__, __func__, __VA_ARGS__); } while (0)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//GLOBAL DEFINITIONS
#define cmdFile "commands.csv"
#define MAX_LEN 4096

//#define DEBUG

//struct for Library calls (Maybe extern or put in header file for callee)
typedef struct {
    char *cmd;
    char *msg;
} Commands;

//filename for Library read/write
char *cmdfile = cmdFile;

//array of commands read/added by user and length of array
Commands *allCommands;
int cmdlen;

//Flags for cleanup/file writing
int new_cmd_flag = 0;
int usr_cmd_file_flag = 0;



//FUNCTIONS

/***************** read_commands ************************************
 * Purpose:
 *  To open and read either user defined command file or default
 *      "commands.csv" file into array struct of Commands
 * 
 * Params:
 *  I char  *filename       string of user defined filename
 * 
 * Returns:
 *  0 - Function executed successfully
 *  1 - File does not exist or could not be opened
 *  2 - Memory could not be allocated for array of structs or strings
*********************************************************************/
int read_commands(char *filename) {
    FILE *fp;

    //Attempt to read passed file name
    if (filename != NULL) {
        fp = fopen(filename, "r");

        if (fp != NULL) {
            cmdfile = (char *)malloc(sizeof(char)*strlen(filename)+1);
            strcpy(cmdfile, filename);
            usr_cmd_file_flag = 1;
        }
    } else {
        fp = fopen(cmdFile, "r");
    }
    
    //if file doesn't exist exit function call
    //temporary
    if (fp == NULL)
        return 1;

    char buffer[MAX_LEN + 1];
    int lines = 0;

    //count number of lines in file
    while(fgets(buffer, MAX_LEN, fp) != NULL)
        lines++;
    
    //reset position in file to beginning
    fseek(fp, 0, SEEK_SET);

    allCommands = (Commands *)malloc(sizeof(Commands)*lines);

    if (allCommands == NULL)
        return 2;

    Commands *tmp = allCommands;
    cmdlen = 0;

    #ifdef DEBUG
        debug_print("Begin reading of file %s\n", cmdfile);
    #endif
    while(fgets(buffer, MAX_LEN, fp) != NULL) {
        int lineSize = strlen(buffer);
        int cmdSize = strcspn(buffer, ",")+2;
	    int msgSize = lineSize-cmdSize-1;

        #ifdef DEBUG
            debug_print("Buffer: \"%s\"\n\tlinesize: %d\t cmdSize: %d\t msgSize: %d\n", buffer, lineSize, cmdSize, msgSize);
        #endif


        tmp->cmd = (char *)malloc(sizeof(char)*cmdSize + 1);
        tmp->msg = (char *)malloc(sizeof(char)*msgSize + 1); 

        if (tmp->cmd == NULL || tmp->msg == NULL)
            return 2;

	    sscanf(buffer, "%[^,],%[^\n]", tmp->cmd, tmp->msg);
	    strcat(tmp->cmd, "\r\n");
	    tmp++; 
        cmdlen++;
    }

    fclose(fp); 
    return 0;
}


/***************** find_command ************************************
 * Purpose:
 *  Helper function to find existing objects in defined array
 * 
 * Params:
 *  I const char  *cmd      string of command to find
 * 
 * Returns:
 *  NULL - Object not found in array
 *  addr - pointer to object in array
*********************************************************************/
Commands * find_command(const char *cmd) {
    
    if (allCommands == NULL || cmd == NULL) {
        return NULL;
    }

    int i;
    Commands *tmp;

    for(i = 0, tmp = allCommands; i < cmdlen; i++, tmp++) {
        if (strcmp(cmd, tmp->cmd) == 0)
            return tmp;
    }

    return NULL;
}


/***************** add_command ************************************
 * Purpose:
 *  Adds a new Commands object to array if that object does not 
 *      already exist
 * 
 * Params:
 *  I const char  *new_cmd       string of new command to add
 *  I const char  *new_msg       string of new message to add
 * 
 * Returns:
 *  0 - Function executed successfully
 *  1 - Unable to reallocate Memory for array or for strings in object
 *  2 - One or more parameters were NULL pointers/Command already existed
*********************************************************************/
int add_command(const char *new_cmd, const char *new_msg) {
    
    // Prevents function from using NULL pointers and adding a command that is already in the list
    // could provide message to user informing if command exists or not
    if (new_cmd == NULL || new_msg == NULL) {
        #ifdef DEBUG
            debug_print("%s","Bad Paramters when adding command");
        #endif
        return 2;
    }

    #ifdef DEBUG
        debug_print("Testing if command is found: %p\n", find_command(new_cmd));
    #endif
    if (find_command(new_cmd) != NULL)
        return 2;

    Commands *new_allCommands = (Commands *)realloc(allCommands, sizeof(Commands)*cmdlen + 1);

    //reallocation failed
    //TODO: Probably should be Fatal error as kernal no longer giving out memory
    //          to process 
    if (new_allCommands == NULL) {
        #ifdef DEBUG
            debug_print("%s","Unable to allocate new memory for commands strucutre");
        #endif
        return 1;
    }

    allCommands = new_allCommands;
    new_allCommands += cmdlen;

    #ifdef DEBUG
        debug_print("Attempting to move to memory address %lu bytes or 0x%lu\n\tStarting Address: %p\n\tEnding Address:   %p\n", sizeof(Commands)*cmdlen, (sizeof(Commands)*cmdlen)/16, allCommands, new_allCommands);
    #endif

    new_allCommands->cmd = (char *)malloc(sizeof(char)*strlen(new_cmd) + 2);
    new_allCommands->msg = (char *)malloc(sizeof(char)*strlen(new_msg));


    //reallocation failed
    //TODO: Probably should be Fatal error as kernal no longer giving out memory
    //          to process 
    if (new_allCommands->cmd == NULL || new_allCommands->msg == NULL) {
        #ifdef DEBUG
            debug_print("Unable to allocate memory for\n\tcmd: %s\n\tmsg: %s", new_cmd, new_msg);
        #endif 
        return 1;
    }

    strcpy(new_allCommands->cmd, new_cmd);
    strcat(new_allCommands->cmd, "\r\n");
    strcpy(new_allCommands->msg, new_msg);

    cmdlen++;
    new_cmd_flag++;

    return 0;
}


/***************** test_command ************************************
 * Purpose:
 *  Test if command exists and return message output to user
 * 
 * Params:
 *  I   const char  *test_cmd       string of command to find
 *  O   char        *outputMsg      pointer to output message to copy into
 *  I/O int         maxOutputLen    length available for outputMsg buffer   
 * 
 * Returns:
 *  0 - Command already exists/Null pointer to output/output size to small
 *  1 - Message copied successfully to outputMsg
 * 
 * Notes/Warnings:
 *  If maxOutputLen is greater than actual allocated memory size for 
 *      outputMsg buffer strncpy will overflow buffer and could cause
 *      undefined behavior - ensure checking of error codes and buffer sizes
*********************************************************************/
int test_command(const char *test_cmd, char *outputMsg, int maxOutputLen) {
    // Functionized finding command    
    Commands *found_cmd = find_command(test_cmd);   

    //Error checking before copying into null pointers
    if (found_cmd == NULL || outputMsg == NULL || maxOutputLen < 0)
        return 0;

    strncpy(outputMsg, found_cmd->msg, maxOutputLen);

    //check if copy was succesful then append "\r\n" 
    if (strcmp(outputMsg, found_cmd->msg) == 0) {
        return 1;
    }

    return 0;
}


/***************** write_commands ************************************
 * Purpose:
 *  Helper function to append new commands added to file
 * 
 * Params:
 *  I int  iNewCmds       number of new commands added
 * 
*********************************************************************/
void write_commands(int iNewCmds) {
    FILE *fp = fopen(cmdfile, "a");

    if (fp == NULL){
        //TODO: PRINT ERROR MSG TO USER (i.e. Tell user that file could not be opened and what commands were not saved)
        return;
    }

    Commands *curr = (allCommands + cmdlen) - iNewCmds;

    #ifdef DEBUG
        debug_print("Attempting to move to memory address %lu bytes or 0x%lu\n\tStarting Address: %p\n\tEnding Address:   %p\n", sizeof(Commands)*(cmdlen - iNewCmds), (sizeof(Commands)*(cmdlen - iNewCmds))/16, allCommands, curr);
    #endif

    int i;

    for(i = 0; i < iNewCmds; i++, curr++) {
        *(curr->cmd + strlen(curr->cmd) - 2) = '\0';
        fprintf(fp, "%s,%s\n", curr->cmd, curr->msg);
    }

    fclose(fp);
    return;
}

/***************** close_commands ************************************
 * Purpose:
 *  Memory clean up and file writing
 * 
 * Returns:
 *  0 - Function executed successfully
 * -1 - allCommands not allocated
*********************************************************************/
int close_commands() {
    
    if (allCommands == NULL)
        return -1;

    if (new_cmd_flag > 0)
        write_commands(new_cmd_flag);

    int i;
    for(i = 0; i < cmdlen; i++) {
        Commands *curr = (allCommands + i);
        free(curr->cmd);
        free(curr->msg);
    }

    free(allCommands);

    if (usr_cmd_file_flag)
        free(cmdfile);
        
    return 0;
}

