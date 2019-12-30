#ifndef CMD_FILE_H
#define CMD_FILE_H

//TODO: Change to Generic after confirming gcc version
int read_commands(char *filename);

int add_command(const char *new_cmd, const char *new_msg);
int test_command(const char *test_cmd, char *outputMsg, int maxOutputLen);
int close_commands();


#endif