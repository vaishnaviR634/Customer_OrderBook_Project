**Description:**

The OrderBook Management System is a command-line application designed to manage a database of customer orders. 

The users need to login and can access their respective database files.

It allows users to add new orders, print existing orders, clear the database, and exit the application.


**Files:**

**Main.c:** Main program file where user interaction and command processing are handled. It includes user authentication and main program flow.

**OrderBook.h:** Header file defining data structures and function prototypes for OrderBook operations. It also contains constants and enums used throughout the project.

**OrderBook.c:** Implementation file for functions related to managing the OrderBook. Handles file operations, memory management, and data serialization.

**Makefile:** Makefile providing compilation instructions for the project. It defines rules to compile Main.c and OrderBook.c into object files and link them to create the executable orderbook.

**passwords.txt:** Sample text file containing username-password-filename tuples for authentication. 


**User Commands:**

make

./orderbook password.txt


**add entry:** Adds a new order to the database. Requires input of Customer ID, Name, Order, and Quantity.

**print OrderBook:** Displays all orders currently stored in the database.

**clear OrderBook:** Deletes all orders from the database.

**:exit :** Terminates the application.


**Functionalities:**

Add Entry:

Users can add a new order by providing Customer ID, Name, Order description, and Quantity. The order is serialized and stored in the database file.

Print OrderBook:

Displays all existing orders stored in the database. 

Clear OrderBook:

Deletes all orders from the database, freeing up memory and resetting the database file.

Authentication:

Users are prompted to enter a username and password at the start of the application. 

The credentials are verified against entries in the passwords.txt file for access to the OrderBook.

Error Handling:

The application handles errors such as file reading/writing issues, command input validation, and memory allocation failures.



