#include <stdio.h>
#include <stdlib.h>
#include "OrderBook.h"


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Provide orderbook file\n");
        exit(EXIT_FAILURE);
    }

    char username[32];
    char password[32];

    // Prompt for username
    printf("Enter username: ");
    fflush(stdout);
    if (fgets(username, sizeof(username), stdin) == NULL) {
        fprintf(stderr, "Error reading username.\n");
        exit(EXIT_FAILURE);
    }
    if (username[strlen(username) - 1] == '\n') {
        username[strlen(username) - 1] = '\0';
    }

    // Prompt for password
    printf("Enter password: ");
    fflush(stdout);
    if (fgets(password, sizeof(password), stdin) == NULL) {
        fprintf(stderr, "Error reading password.\n");
        exit(EXIT_FAILURE);
    }
    if (password[strlen(password) - 1] == '\n') {
        password[strlen(password) - 1] = '\0';
    }
     char*  user;
    char* str = checker(argv[1], username, password, user);
    OrderBook* OrderBook1 = open_OrderBook(str);

    while (true) {
        printf("%s$ ", user);
        InpBuffer* inp1 = (InpBuffer*)calloc(1, sizeof(InpBuffer));
        inp1->buffer = NULL;
        inp1->buffer_length = 32;
        readinp(inp1);

        if (strncmp(inp1->buffer, ":exit", 5) == 0) {
            OrderBook_close(OrderBook1);
            free(inp1->buffer);
            free(inp1);
            exit(EXIT_SUCCESS);
        }

        Command* C = set_command(inp1);
        Exec E = execute(inp1, C, OrderBook1);
        if (E == 0)
            printf("Executed\n");
        else if (E == 1)
            printf("Unrecognized command\n");
        else if (E == 2)
            printf("OrderBook Full\n");
        else if (E == 3)
            printf("String size limit exceeded\n");
        else
            printf("Execution error, check arguments\n");

        free(C);
    }

    OrderBook_close(OrderBook1);
    free(OrderBook1);

    return 0;
}
