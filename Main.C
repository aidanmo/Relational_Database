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
    PREPARE_UNRECOGNIZED_STATEMENT
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

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define sizeOfAttr(type, attr) sizeof(((type*)0)->attr)

//TO-DO 1: Add additional rows to single table
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

const uint32_t ID_SIZE = sizeOfAttr(Row, id);
const uint32_t USERNAME_SIZE = sizeOfAttr(Row, username);
const uint32_t EMAIL_SIZE = sizeOfAttr(Row, email);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t PAGE_SIZE = 4096;
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

typedef struct
{
    StatementType type;
    Row rowToInsert;
} Statement;

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
        int argsAssigned = sscanf(inputBuffer->buffer, "insert %d %s %s", &(statement->rowToInsert.id),),
            statement->rowToInsert.username, statement->rowToInsert.email);
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

void executeStatement(Statement* statement)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        printf("This is where we would do an insert.\n");
        break;
    case (STATEMENT_SELECT):
        printf("This is where we would do a select.\n");
        break;
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
        }

        // Need to define this function and statement variable.
        executeStatement(&statement);
        printf("Executed.\n");
    }
}