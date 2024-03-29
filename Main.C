#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defining enums
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR

} PrepareResult;

// Defining new data type called "InputBuffer"
// InputBuffer is defined as a stuct which can hold a collection of variables of different types.
// It's used to buffer input. The buffer can store input temporarily.
typedef struct
{ // pointer
    char* buffer;
    size_t bufferLength;
    // Signed interger which represents the size of an object in  bytes.
    ssize_t inputLength;
} InputBuffer;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum {
    EXECUTE_SUCCESS,
	EXECUTE_TABLE_FULL,
} ExecuteResult;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define sizeOfAttr(type, attr) sizeof(((type*)0)->attr)

//TO-DO 1: Add additional rows to single table
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

//TO-DO 1: Add aditional rows here as well
const uint32_t ID_SIZE = sizeOfAttr(Row, id);
const uint32_t USERNAME_SIZE = sizeOfAttr(Row, username);
const uint32_t EMAIL_SIZE = sizeOfAttr(Row, email);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

typedef struct
{
    StatementType type;
    Row rowToInsert;
} Statement;

void printRow(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

//TODO 1
void serializeRow(Row* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy((char*)destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    memcpy((char*)destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);

}
//TODO 1
void deserializeRow(void* source, Row* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), (char*)source + EMAIL_OFFSET, EMAIL_SIZE); 
}

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


typedef struct{
    uint32_t numRows;
    //void use to represent a pointer to a unknown data type.
    void* pages[TABLE_MAX_PAGES];
} Table;

void* rowSlot(Table* table, uint32_t rowNum) {
    uint32_t pageNum = rowNum / ROWS_PER_PAGE;
    void* page = table->pages[pageNum];
    if(page == NULL) {
        //allocate memory only when we try to access page.
        page = table->pages[pageNum] = malloc(PAGE_SIZE);
    }
    uint32_t rowOffset = rowNum % ROWS_PER_PAGE;
    uint32_t byteOffset = rowOffset * ROW_SIZE;
    return (char*)page + byteOffset;

}

Table* newTable() {
    Table* table = (Table*)malloc(sizeof(Table));
    table->numRows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }
    return table;
}

void freeTable (Table* table) {
    for (int i = 0; table->pages[i]; i++) {
        free(table->pages[i]);
    }
    free(table);
}



/// @brief
/// @return returns inputBuffer which is a pointer to a new inputBuffer object
InputBuffer* newInputBuffer()
{
    // malloc dynamically allocates memory to the inputBuffer object
    InputBuffer* inputBuffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->bufferLength = 0;
    inputBuffer->inputLength = 0;

    return inputBuffer;
}

MetaCommandResult doMetaCommand(InputBuffer* inputBuffer)
{
    if (strcmp(inputBuffer->buffer, ".exit") == 0)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepareStatement(InputBuffer* inputBuffer,
                               Statement* statement)
{
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
        int argsAssigned = sscanf(
            inputBuffer->buffer, "insert %d %s %s", &(statement->rowToInsert.id),
            statement->rowToInsert.username, statement->rowToInsert.email);
        if (argsAssigned < 3) {
        return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }

    if (strcmp(inputBuffer->buffer, "select") == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult executeInsert(Statement* statement, Table* table) {
    if (table->numRows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }
    Row* rowToInsert = &(statement->rowToInsert);

    serializeRow(rowToInsert, rowSlot(table, table->numRows));
    table->numRows += 1;
    
    return EXECUTE_SUCCESS;
}

ExecuteResult executeSelect(Statement* statement, Table* table) {
    Row row;
    for (uint32_t i = 0; i < table->numRows; i++) {
        deserializeRow(rowSlot(table, i), &row);
        printRow(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult executeStatement(Statement* statement, Table* table)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return executeInsert(statement, table);
    case (STATEMENT_SELECT):
        return executeSelect(statement, table);
    }
}

void printPrompt() { printf("db > "); }

void readInput(InputBuffer* inputBuffer)
{
    // assigns number of bytes read from stdin (user input from cmdline)
    ssize_t bytes_read =
        getline(&(inputBuffer->buffer), &(inputBuffer->bufferLength), stdin);

    // Error handling for byte_read values of 0 or a negative value.
    if (bytes_read <= 0)
    {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // The getline function reads the entire line, including the trailing newline. Subtracting 1 byte from the bytes read
    // will provide the correct length
    inputBuffer->inputLength = bytes_read - 1;

    // This method is used to terminate the end of the buffer attribute associated with our structInput buffer.
    // The buffer attribute points to an array of chars, in order to terminate this array representing a string we must set
    // the last value to zero. This is done in C to represent the end of a string.
    inputBuffer->buffer[bytes_read - 1] = 0;
}

void closeInputBuffer(InputBuffer* inputBuffer)
{
    // In order to prevent memory leaks we must release the memory allocated for the inputBuffer and it's buffer after
    // we no longer need it.
    if (inputBuffer != NULL)
    {
        free(inputBuffer->buffer);
        free(inputBuffer);
        // Assigning inputerBuffer pointer to a null value after it's no longer needed.
        inputBuffer = NULL;
    }
}

int main(int argc, char* argv[])
{
    Table* table = newTable();
    InputBuffer* inputBuffer = newInputBuffer();
    while (true)
    {
        printPrompt();
        readInput(inputBuffer);

        if (inputBuffer->buffer[0] == '.')
        {
            switch (doMetaCommand(inputBuffer))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized command '%s'\n", inputBuffer->buffer);
                continue;
            }
        }

        Statement statement;
        switch (prepareStatement(inputBuffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            printf("Unrecognized keyword at start of '%s' ,\n", inputBuffer->buffer);
            continue;
        case (PREPARE_SYNTAX_ERROR):
            printf("Syntax error within '%s' ,\n", inputBuffer->buffer);
        }

        // Need to define this function and statement variable.
        switch (executeStatement(&statement, table)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
        }
        
    }
}