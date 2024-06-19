#include "OrderBook.h"



#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
const uint32_t ID_SIZE = size_of_attribute(Row, CustomerID);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, Name);
const uint32_t ORDER_SIZE = size_of_attribute(Row, Order);
const uint32_t Q_SIZE = size_of_attribute(Row, Quantity);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t ORDER_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t Q_OFFSET = ORDER_OFFSET + ORDER_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + ORDER_SIZE + Q_SIZE;
const uint32_t PAGE_SIZE = 2048;
#define OrderBook_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = 2048 / ROW_SIZE;
const uint32_t OrderBook_MAX_ROWS = ROWS_PER_PAGE * 100;

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num > 100) {
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->size / PAGE_SIZE;

        // partial page
        if (pager->size % PAGE_SIZE) {
            num_pages += 1;
        }

        if (page_num <= num_pages) {
            lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t size1 = read(pager->fd, page, PAGE_SIZE);
            if (size1 == -1) {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;
    }

    return pager->pages[page_num];
}

void* ptr_to_row(OrderBook* OrderBook1, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    void* page = get_page(OrderBook1->pager, page_num);

    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

void readinp(InpBuffer* inp) {
    ssize_t inp_read_size = getline(&(inp->buffer), &(inp->buffer_length), stdin);
    if (inp_read_size <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    inp->inp_size = inp_read_size - 1; // newline
    inp->buffer[inp_read_size - 1] = 0; // str 0
}

void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->CustomerID), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->Name), USERNAME_SIZE);
    memcpy(destination + ORDER_OFFSET, &(source->Order), ORDER_SIZE);
    memcpy(destination + Q_OFFSET, &(source->Quantity), Q_SIZE);
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->CustomerID), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->Name), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->Order), source + ORDER_OFFSET, ORDER_SIZE);
    memcpy(&(destination->Quantity), source + Q_OFFSET, Q_SIZE);
}

Command* set_command(InpBuffer* inp1) {
    Command* C = (Command*)malloc(sizeof(Command));
    if (strncmp(inp1->buffer, "add entry", 9) == 0) {
        C->type = ADD;
    } else if (strcmp(inp1->buffer, "print OrderBook") == 0) {
        C->type = PRINT;
    } else if (strcmp(inp1->buffer, "clear OrderBook") == 0) {
        C->type = CLEAR;
    } else {
        C->type = INVALID;
    }
    return C;
}

Exec add_command(InpBuffer* inp, Command* c, OrderBook* t) {
    if (t->rows_count >= OrderBook_MAX_ROWS) {
        return OrderBook_FULL;
    }

    char* keyword = strtok(inp->buffer, " ");
    char* Keyword1 = strtok(NULL, " ");
    char* id = strtok(NULL, " ");
    char* name = strtok(NULL, " ");
    char* order = strtok(NULL, " ");
    char* q = strtok(NULL, " ");

    if (id == NULL || name == NULL || order == NULL || q == NULL) {
        return STRING_TOO_LONG;
    }

    if (strlen(name) > 31) {
        return STRING_TOO_LONG;
    }
    if (strlen(order) > 63) {
        return STRING_TOO_LONG;
    }

    c->data.CustomerID = atoi(id);
    c->data.Quantity = atoi(q);
    strcpy(c->data.Name, name);
    strcpy(c->data.Order, order);

    Row* r1 = &(c->data);
    serialize_row(r1, ptr_to_row(t, t->rows_count));
    t->rows_count++;
    return SUCCESS;
}

Exec print_command(InpBuffer* inp, Command* c, OrderBook* t) {
    Row row;
    for (uint32_t i = 0; i < t->rows_count; i++) {
        deserialize_row(ptr_to_row(t, i), &row);
        printf(" %d %s %s %d\n", row.CustomerID, row.Name, row.Order, row.Quantity);
    }
    return SUCCESS;
}

Exec del_command(InpBuffer* inp, Command* c, OrderBook* t) {
    printf("Are you sure? \n");
    char* reply = (char*)malloc(100 * sizeof(char));
    size_t s = 100;
    ssize_t size = getline(&(reply), &s, stdin);
    if (strcmp(reply, "Yes\n") == 0) {
        printf("Deleting...\n");
        // delete OrderBook
        for (int i = 0; i < 100; i++) {
            if (t->pager->pages[i] != NULL)
                free(t->pager->pages[i]);
        }
        t->rows_count = 0;
    } else {
        printf("Abort deletion\n");
    }
    free(reply);
    return SUCCESS;
}

Exec execute(InpBuffer* inp, Command* c, OrderBook* t) {
    switch (c->type) {
        case INVALID:
            return UNRECOGNIZED_COMMAND;
        case ADD:
            return add_command(inp, c, t);
        case PRINT:
            return print_command(inp, c, t);
        case CLEAR:
            return del_command(inp, c, t);
        default:
            return EXECUTION_ERROR;
    }
}

OrderBook* open_OrderBook(char* file) {
    int fd = open(file, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        exit(EXIT_FAILURE);
    }

    Pager* pager = malloc(sizeof(Pager));
    pager->fd = fd;
    pager->size = lseek(fd, 0, SEEK_END);

    for (uint32_t i = 0; i < 100; i++) {
        pager->pages[i] = NULL;
    }

    OrderBook* OrderBook1 = (OrderBook*)malloc(sizeof(OrderBook));
    OrderBook1->rows_count = pager->size / ROW_SIZE;
    OrderBook1->pager = pager;
    return OrderBook1;
}

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == NULL) {
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        exit(EXIT_FAILURE);
    }
    ssize_t size1 = write(pager->fd, pager->pages[page_num], size);

    if (size1 == -1) {
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

void OrderBook_close(OrderBook* t) {
    Pager* pager = t->pager;
    if (t->rows_count == 0) {
        int truncate_result = ftruncate(pager->fd, 0);
        if (truncate_result == -1) {
            perror("Failed to truncate file");
            exit(EXIT_FAILURE);
        }
        return;
    }

    uint32_t full_pages = t->rows_count / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < full_pages; i++) {
        if (pager->pages[i] != NULL) {
            pager_flush(pager, i, PAGE_SIZE);
            pager->pages[i] = NULL;
            free(pager->pages[i]);
        }
    }

    uint32_t rows_in_partial_page = t->rows_count % ROWS_PER_PAGE;
    if (rows_in_partial_page > 0) {
        if (pager->pages[full_pages] != NULL) {
            pager_flush(pager, full_pages, rows_in_partial_page * ROW_SIZE);
            pager->pages[full_pages] = NULL;
            free(pager->pages[full_pages]);
        }
    }

    int fd1 = close(pager->fd);
    if (fd1 == -1) {
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < 100; i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(t);
}

char* checker(char* password_file, char* username, char* password, char* user) {
    FILE* fp = fopen(password_file, "r");
    if (fp == NULL) {
        perror("Error opening password file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    char stored_password[32];
    int found = 0;
    char file[100];
    char* str = NULL;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s %s %s", user, stored_password, file) == 3) {
            if (strcmp(username, user) == 0 && strcmp(password, stored_password) == 0) {
                found = 1;
                str = strdup(file);
                break;
            }
        }
    }

    fclose(fp);
    if (!found) {
        printf("Username and password not found or incorrect.\n");
        exit(EXIT_FAILURE);
    }

    return str;
}
