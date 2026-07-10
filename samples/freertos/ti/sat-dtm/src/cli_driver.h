/*
 *  ======== cli_driver.h ========
 *  This file implements a basic CLI driver that allows dynamic registration
 *  of commands and processes input to execute associated functions.
 *
 *  Originally from the TI CLI_driver_LP_EM_CC2340R5_freertos_ticlang example.
 */

#ifndef CLI_DRIVER_H
#define CLI_DRIVER_H

#include <stdint.h>
#include <stddef.h>

/* Command structure */
typedef struct {
	const char *keyword;
	void (*function)(int argc, char **argv);
	const char *resultMessage;
	const char *helpMessage;
	int argCount;
} CLI_Command;

/* CLI Driver API */
void CLI_init(void);
void CLI_print(const char *msg);
void CLI_addCommand(const char *keyword, void (*function)(int argc, char **argv),
		    const char *resultMessage, const char *helpMessage,
		    int argCount);
void CLI_processInput(uint8_t *inputBuffer, size_t bufferSize, size_t bufferCapacity);

/* Macro for adding commands */
#define CLI_ADD_COMMAND(keyword, function, resultMessage, helpMessage, argCount) \
	CLI_addCommand(#keyword, function, resultMessage, helpMessage, argCount)

#endif /* CLI_DRIVER_H */
