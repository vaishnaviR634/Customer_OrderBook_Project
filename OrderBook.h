#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#pragma once 

typedef struct {
    uint32_t CustomerID;
    char Name[32];
    char Order[64];
    uint32_t Quantity;
} Row;

typedef struct {
    uint32_t size;
    void* pages[100];
    int fd;
} Pager;

typedef struct {
    uint32_t rows_count;
    Pager* pager;
} OrderBook;

typedef struct {
    char* buffer;
    ssize_t buffer_length;
    ssize_t inp_size;
} InpBuffer;

typedef enum {
    SUCCESS,
    UNRECOGNIZED_COMMAND,
    OrderBook_FULL,
    STRING_TOO_LONG,
    EXECUTION_ERROR
} Exec;

typedef enum {
    ADD,
    PRINT,
    CLEAR,
    INVALID
} Command_list;

typedef struct {
  Command_list type;
  Row data;
} Command;


OrderBook* open_OrderBook(char* file);
void OrderBook_close(OrderBook* t);
char* checker(char* password_file, char* username, char* password, char* user);
void readinp(InpBuffer* inp);
void* get_page(Pager* pager, uint32_t page_num);
void* ptr_to_row(OrderBook* OrderBook1, uint32_t row_num);
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
Command* set_command(InpBuffer* inp1);
Exec add_command(InpBuffer* inp, Command* c, OrderBook* t);
Exec print_command(InpBuffer* inp, Command* c, OrderBook* t);
Exec del_command(InpBuffer* inp, Command* c, OrderBook* t);
Exec execute(InpBuffer* inp, Command* c, OrderBook* t);


