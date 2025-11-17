#include <stdio.h>    
#include <stdlib.h>    
#include "helper.h"   

int main(void) {
    printf("Calling helper function...\n");
    printf("Hello, World!\n");

    int result = helper_function();
    if (result == -1) {
        perror("helper_function failed");
        exit(EXIT_FAILURE);
    }

    printf("helper_function returned: %d\n", result);

    return 0;
}
