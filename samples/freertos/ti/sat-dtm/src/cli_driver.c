/*
 *  ======== cli_driver.c ========
 *  This file implements a basic CLI driver that allows dynamic registration
 *  of commands and processes input to execute associated functions.
 *
 *  Originally from the TI CLI_driver_LP_EM_CC2340R5_freertos_ticlang example.
 */

#include "cli_driver.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ti/drivers/UART2.h>

/* Maximum number of commands that can be registered */
#define MAX_COMMANDS 10

/* Maximum number of arguments a command can take */
#define MAX_ARGS     5

/* Array to store registered commands */
static CLI_Command *commandList[MAX_COMMANDS];
static size_t commandCount = 0;

/* UART handle for sending messages */
extern UART2_Handle uart;

void CLI_print(const char *msg)
{
	if (msg != NULL) {
		UART2_write(uart, msg, strlen(msg), NULL);
	}
}

/* Function to clear the UART terminal */
static void CLI_clearScreen(void)
{
	/* ANSI escape sequence to clear the screen and move the cursor to the top-left corner */
	CLI_print("\033[2J\033[H");
}

/* Wrapper for CLI_clearScreen to match the expected function signature */
static void CLI_clearScreenWrapper(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	CLI_clearScreen();
}

/* Function to print help information */
static void CLI_printHelp(void)
{
	CLI_print("\r\nAvailable Commands:\r\n");

	/* Iterate through all registered commands and print their details */
	for (size_t i = 0; i < commandCount && i < MAX_COMMANDS; i++) {
		CLI_Command *cmd = commandList[i];

		/* Skip invalid commands */
		if (cmd == NULL || cmd->keyword == NULL ||
		    cmd->helpMessage == NULL) {
			continue;
		}

		/* Print the command's keyword and help message directly */
		CLI_print(cmd->keyword);
		CLI_print(": ");
		CLI_print(cmd->helpMessage);
		CLI_print("\r\n");
	}
}

/* Wrapper for CLI_printHelp to match the expected function signature */
static void CLI_printHelpWrapper(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	CLI_printHelp();
}

/* Initialize the CLI driver */
void CLI_init(void)
{
	/* Reset count and clear the command list */
	commandCount = 0;
	memset(commandList, 0, sizeof(commandList));

	/* Add the built-in help and clear commands */
	CLI_addCommand(
		"help", CLI_printHelpWrapper, "\r\nHelp Command Executed\r\n",
		"Displays all available commands and their descriptions", 0);
	CLI_addCommand("clear", CLI_clearScreenWrapper, "\r\nScreen Cleared\r\n",
		       "Clears the UART terminal screen", 0);
}

/* Add a command to the CLI */
void CLI_addCommand(const char *keyword, void (*function)(int argc, char **argv),
		    const char *resultMessage, const char *helpMessage,
		    int argCount)
{
	/* Validate inputs */
	if (commandCount >= MAX_COMMANDS || keyword == NULL ||
	    function == NULL || resultMessage == NULL || helpMessage == NULL) {
		CLI_print("Invalid command parameters or max commands "
			  "reached\r\n");
		return;
	}

	/* Allocate memory for the new command */
	CLI_Command *newCommand = (CLI_Command *)malloc(sizeof(CLI_Command));
	if (newCommand == NULL) {
		CLI_print("Failed to add command due to memory allocation "
			  "error\r\n");
		return;
	}

	/* Directly assign the pointers instead of duplicating the strings */
	newCommand->keyword = keyword;
	newCommand->resultMessage = resultMessage;
	newCommand->helpMessage = helpMessage;
	newCommand->function = function;
	newCommand->argCount = argCount;

	/* Add the command to the list and increment the command count */
	if (commandCount < MAX_COMMANDS) {
		commandList[commandCount++] = newCommand;
	} else {
		CLI_print("Max commands reached. Cannot add command\r\n");

		/* Free memory if the command cannot be added */
		free(newCommand);
	}
}

/*
 * Process input and execute matching command.
 * bufferSize is the number of valid input bytes; bufferCapacity is the
 * total allocated size of inputBuffer. Returns without action if
 * inputBuffer is NULL or bufferSize does not leave room for a null
 * terminator (i.e. bufferSize >= bufferCapacity).
 */
void CLI_processInput(uint8_t *inputBuffer, size_t bufferSize, size_t bufferCapacity)
{
	/* Cast the input buffer to a string and null-terminate it */
	char *input = (char *)inputBuffer;
	if (inputBuffer == NULL || bufferSize >= bufferCapacity) {
		return;
	}
	input[bufferSize] = '\0';

	/* Array to store command arguments */
	char *argv[MAX_ARGS + 1]; /* +1 to include the command keyword itself */
	int argc = 0;

	/* Tokenize the input string into command and arguments */
	char *saveptr;
	char *token = strtok_r(input, " ", &saveptr); /* Split the input by spaces */
	while (token != NULL && argc < MAX_ARGS + 1) {
		argv[argc++] = token; /* Store each token in the argv array */
		token = strtok_r(NULL, " ", &saveptr); /* Get the next token */
	}

	/* If no command is entered, return early */
	if (argc == 0) {
		CLI_print("No command entered\r\n");
		return;
	}

	/* Iterate through the list of registered commands to find a match */
	for (size_t i = 0; i < commandCount && i < MAX_COMMANDS; i++) {
		CLI_Command *cmd = commandList[i];

		/* Check if the entered command matches the registered command keyword */
		if (cmd != NULL && strcmp(argv[0], cmd->keyword) == 0) {
			/* Validate the number of arguments provided */
			if (argc - 1 != cmd->argCount) {
				CLI_print("Invalid number of arguments\r\n");
				return;
			}

			/* Execute the command's associated function with the arguments */
			cmd->function(
				argc - 1,
				&argv[1]); /* Pass arguments (excluding the command keyword) */

			/* Send the result message to indicate successful execution */
			CLI_print(cmd->resultMessage);
			return;
		}
	}

	/* If no matching command is found, send an error message */
	CLI_print("Invalid Command\r\n");
}
