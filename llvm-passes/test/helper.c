#include <string.h>   
#include <unistd.h>  
#include "helper.h"

int helper_function(void) {
    const char *msg = "Hello from helper_function!\n";

    ssize_t written = write(STDOUT_FILENO, msg, strlen(msg));
    if (written < 0) {
        return -1;   
    }

    return (int) written;  