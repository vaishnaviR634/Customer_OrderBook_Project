#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

char* user=NULL;

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


typedef struct {
  uint32_t rows_count;
  Pager* pager;
} OrderBook;


void* get_page(Pager* pager, uint32_t page_num) {
 if (page_num > 100) { exit(EXIT_FAILURE);
  }

  if (pager->pages[page_num] == NULL) {
    // Cache miss
    void* page = malloc(PAGE_SIZE);
    uint32_t num_pages = pager->size / PAGE_SIZE;

    //partial page
    if (pager->size % PAGE_SIZE) {
      num_pages += 1;
    }

    if (page_num <= num_pages) {
      lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t size1 = read(pager->fd, page, PAGE_SIZE);
      if (size1 == -1) { printf("Error reading file: %d\n", errno);
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



typedef enum {
  SUCCESS,
  UNRECOGNIZED_COMMAND,
  OrderBook_FULL,
  STRING_TOO_LONG,
  EXECUTION_ERROR
} Exec;

typedef enum { ADD, PRINT, CLEAR, INVALID } Command_list;

typedef struct {
  Command_list type;
  Row data;
} Command;

typedef struct{
char* buffer;
ssize_t buffer_length;
ssize_t inp_size;
}InpBuffer;

void readinp(InpBuffer* inp) {
     ssize_t inp_read_size = getline(&(inp->buffer), &(inp->buffer_length), stdin);
  if (inp_read_size <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  inp->inp_size = inp_read_size - 1; //newline
  inp->buffer[inp_read_size - 1] = 0; //str 0
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


Command* set_command(InpBuffer* inp1){
    Command* C =(Command*)malloc(sizeof(Command));
    if(strncmp(inp1->buffer,"add entry",9)==0){C->type=ADD; }
    else if(strcmp(inp1->buffer,"print OrderBook")==0){C->type=PRINT; }
    else if(strcmp(inp1->buffer,"clear OrderBook")==0){C->type=CLEAR; }
    else C->type=INVALID;
    return C;

}

Exec add_command(InpBuffer* inp,Command* c, OrderBook* t){

    if(t->rows_count >= OrderBook_MAX_ROWS){return OrderBook_FULL;}

  char* keyword = strtok(inp->buffer, " ");
  char* Keyword1 = strtok(NULL," ");
  char* id = strtok(NULL, " ");
  char* name = strtok(NULL, " ");
  char* order = strtok(NULL, " ");
  char* q = strtok(NULL, " ");

  if (id == NULL || name == NULL || order == NULL || q==NULL) {
    return 3;
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
  strcpy(c-> data.Order, order);


    Row* r1 = &(c->data);
    serialize_row( r1, ptr_to_row(t,t->rows_count));
    t->rows_count++;
    return 0;
    
    }

Exec print_command(InpBuffer* inp, Command* c,  OrderBook* t){
Row row;
  for (uint32_t i = 0; i < t->rows_count; i++) {
     deserialize_row(ptr_to_row(t, i), &row);
     printf(" %d %s %s %d\n",row.CustomerID, row.Name, row.Order, row.Quantity);
  }
    return 0;}

Exec del_command(InpBuffer* inp, Command* c, OrderBook* t){
    printf("Are you sure? \n");
    char* reply=(char*)malloc(100 * sizeof(char));
    size_t  s=100;
    ssize_t size=getline(&(reply),&s,stdin);
    if(strcmp(reply,"Yes\n")==0){printf("Deleting...\n");
    // delete OrderBook  
    for (int i = 0;i<100; i++) {
	if(t->pager->pages[i]!=NULL)free(t->pager->pages[i]);
    }
    t->rows_count = 0;
    }

    else printf("Abort deletion\n");
    free(reply);
    return 0;
    }

Exec execute(InpBuffer* inp, Command* c, OrderBook* t){
switch (c->type) {
    case INVALID: 
        return 1;
    case ADD:
        return add_command(inp,c,t);
    case PRINT:
        return print_command(inp,c,t);
    case CLEAR:
        return del_command(inp,c,t);
    default:
        return 1;
}

}

OrderBook* open_OrderBook(char* file) {
    //open_file
     int fd = open(file, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if(fd==-1){exit(EXIT_FAILURE);}

    Pager* pager = malloc(sizeof(Pager));
    pager->fd = fd;
    pager->size = lseek(fd, 0, SEEK_END);

  for (uint32_t i = 0; i < 100; i++) {
    pager->pages[i] = NULL;
  }

 OrderBook* OrderBook1 = (OrderBook*)malloc(sizeof(OrderBook));
  OrderBook1->rows_count = pager->size/ROW_SIZE;
  OrderBook1->pager= pager;
  //printf("%d\n",);
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


    void OrderBook_close(OrderBook* t){
        Pager* pager = t->pager;
        if(t->rows_count==0){
        int truncate_result = ftruncate(pager->fd, 0);
        if (truncate_result == -1) {
            perror("Failed to truncate file");
            exit(EXIT_FAILURE);
        }
        return;
        }
  uint32_t full_pages = t->rows_count / ROWS_PER_PAGE;
  for (uint32_t i=0;i<full_pages;i++) {
    if (pager->pages[i] != NULL) {
      pager_flush(pager, i, PAGE_SIZE);
    //update in file

    pager->pages[i] = NULL;
      free(pager->pages[i]);
    }
  }

//partial pages 
  uint32_t rows_in_partial_page = t->rows_count % ROWS_PER_PAGE;
  if (  rows_in_partial_page > 0) {
    if (pager->pages[full_pages] != NULL) {
      pager_flush(pager, full_pages, rows_in_partial_page * ROW_SIZE);
      
      pager->pages[full_pages] = NULL;
      free(pager->pages[full_pages]);
    }
  }

  int fd1 = close(pager->fd);
 if (fd1 == -1) { exit(EXIT_FAILURE);
 }

  for (uint32_t i = 0; i < 100; i++) {
    void* page = pager->pages[i];
    if (page) { free(page); pager->pages[i] = NULL;
    }
  }
  free(pager);
  free(t);
    }

char* checker(char* password_file, char* username, char* password) {
    FILE *fp = fopen(password_file, "r");
    if (fp == NULL) {
        perror("Error opening password file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    char stored_username[32], stored_password[32];
    int found = 0;
    char file[100];

    char* str = NULL;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s %s %s", stored_username, stored_password, file) == 3) {
            if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
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
    
    user=strdup(stored_username);

    return str;
}




int main(int argc, char* argv[]){
if(argc<2){printf("Provide orderbook file\n"); exit(EXIT_FAILURE);}

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

      char* str=checker(argv[1], username, password);

      OrderBook* OrderBook1 = open_OrderBook(str);

while(true){
printf("%s$ ",user);
InpBuffer* inp1 = (InpBuffer*)calloc(1, sizeof(InpBuffer));
inp1->buffer=NULL;
inp1->buffer_length=32;
readinp(inp1);
if(strncmp(inp1->buffer,":exit",5)==0){
    OrderBook_close(OrderBook1);
   free(inp1->buffer);
   free(inp1);
   exit(EXIT_SUCCESS); 
}
Command* C= set_command(inp1);
Exec E=execute(inp1,C,OrderBook1);
if(E==0)printf("Executed\n");
else if(E==1)printf("Unrecognized command\n");
else if(E==2)printf("OrderBook Full\n");
else if(E==3)printf("String size limit exceeded\n");
else printf("Execution error, check arguments\n");
free(C);

}

OrderBook_close(OrderBook1);
  free(OrderBook1);

}