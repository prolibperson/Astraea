#include "shell.h"
#include "terminal.h"
#include "libc.h"
#include "keyboard.h"
#include "cpu_manager.h"

#define HISTORY_SIZE 10

/* command argument variable */
char* command_arguments = NULL;

/* all commands as command name, description and respective function */
static const shell_command_t commands[] = {
    { "help", "List all available commands", shell_help },
    { "echo", "Print text to the terminal",  shell_echo },
    { "clear","Clear the terminal",          shell_clear },
    { "reboot","Reboot the system",          shell_reboot },
    { "rng", "Print a randomly generated number",        shell_rng },
    { "info", "Prints information about the OS and system", shell_info },
};

/* how many commands there are */
static const size_t command_count = sizeof(commands) / sizeof(commands[0]);

/* command history variables */
static char history[HISTORY_SIZE][256];
static int history_count = 0;
static int history_index = -1;

/* SHELL COMMAND FUNCTIONS */

/* prints neofetch looking thing */
void shell_info(void)
{
    for (size_t i = 0; i < 10; i++) {
        terminal_setcolor_gradient(i * 2);
        tprintf("\n");
        switch (i) {
            case 0: tprintf("         .8.           "); terminal_setcolor(0xCC00CC); tprintf("AstraeaOS"); terminal_setcolor_gradient(i * 2); break;
            case 1: tprintf("        .888.          "); break;
            case 2: tprintf("       :88888.         "); terminal_setcolor(0xCC00CC); tprintf("Version v0.1.1"); terminal_setcolor_gradient(i * 2); break;
            case 3: tprintf("      . `88888.       "); break;
            case 4: tprintf("     .8. `88888.      "); break;
            case 5: tprintf("    .8`8. `88888.     "); break;
            case 6: tprintf("   .8' `8. `88888.    "); break;
            case 7: tprintf("  .8'   `8. `88888.   "); break;
            case 8: tprintf(" .888888888. `88888.  "); break;
            case 9: tprintf(".8'       `8. `88888."); break;
        }
    }
    tprintf("\n");
}

/* rng */
void shell_rng(void)
{
    /* print result of rand() */
    tprintf("%d", rand());
}

/* reboot system */
void shell_reboot(void) {
    tprintf("Rebooting system...\n");

    /* clear interrupts and trigger cpu reset */
    asm volatile (
        "cli\n"
        "movb $0xFE, %al\n" 
        "outb %al, $0x64\n"
        "hlt\n"
    );
}

/* clear terminal */
void shell_clear(void) {
    terminal_clear(); 
}

/* print help with command struct */
void shell_help(void) {
    tprintf("Available commands:\n");
    for (size_t i = 0; i < command_count; i++) {
        tprintf(" - ");
        tprintf(commands[i].name);
        tprintf(": ");
        tprintf(commands[i].description);
        terminal_putchar('\n');
    }
}

/* echo echo echo echo echo echo echo */
void shell_echo(void) {
    if (command_arguments && *command_arguments) {
        tprintf(command_arguments);
    } else {
        tprintf("\n");
    }
    terminal_putchar('\n');
}

/* SHELL FUNCTIONS */

/* clear input field */
void clear_input_field(size_t input_len) {
    /* for loop that clears the characters and backspaces */
    for (size_t i = 0; i < input_len; i++) {
        terminal_putchar('\b');
        terminal_putchar(' ');
        terminal_putchar('\b');
    }
}

/* check for if the input is only spaces */
int is_input_only_spaces(const char* input, size_t input_len) {
    /* iterate and check */
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

/* print the user@AstraeaOS $ */
void print_shell_prompt() 
{
    terminal_setcolor(0xFF00FF);
    tprintf("\nuser");
    terminal_setcolor(0xCC00CC);
    tprintf("@");

    terminal_setcolor(0xFF00FF);  
    tprintf("AstraeaOS");

    terminal_setcolor(0xCC00CC);
    tprintf(" $ ");
    terminal_setcolor(0xFF00FF);
}

/* backspace handlers decreases input length */
void handle_backspace(char *input, size_t *input_len) {
    if (*input_len > 0) {
        (*input_len)--;
        terminal_putchar('\b');
    }
}

/* handle history when going up */
void handle_history_up(char *input, size_t *input_len) {
    /* if its bigger than 0 */
    if (history_count > 0) {
        /* make sure it doesnt go negative */
        if (history_index == -1) {
            history_index = 0;
        } 
        /* if it aint negative increase the history_index */
        else if (history_index < history_count - 1) {
            history_index++;
        }
        /* copy input into the history */
        strcpy(input, history[history_count - 1 - history_index]);
        *input_len = strlen(input);

        /* clear input field, print shell prompt and print input */
        clear_input_field(*input_len);
        print_shell_prompt();
        tprintf(input);
    }
}

/* handle history when going down im yelling timber you better move you better dance */
void handle_history_down(char *input, size_t *input_len) {
    /* if bigger than 0*/
    if (history_index > 0) {
        /* decrease */
        history_index--;

        /* copy input into history */
        strcpy(input, history[history_count - 1 - history_index]);

        /* update input length */
        *input_len = strlen(input);

        /* clear input field, print shell prompt and print input */
        clear_input_field(*input_len);
        print_shell_prompt();
        tprintf(input);
    } else if (history_index == 0) {
        history_index = -1;
        input[0] = '\0';

        clear_input_field(*input_len);
        *input_len = 0;
        print_shell_prompt();
    }
}

/* process input! */
void process_input(char *input, size_t input_len) {
    /* if its empty just print newline and return */
    if (input_len == 0 || is_input_only_spaces(input, input_len)) {
        tprintf("\n");
        return;
    }

    /* null terminate */
    input[input_len] = '\0';

    /* if input is unique, store in history */
    if (input_len > 0 && (history_count == 0 || strcmp(history[history_count - 1], input) != 0)) {
        /* if history count smaller than history size */
        if (history_count < HISTORY_SIZE) {
            /* strcpy and increment history count */
            strcpy(history[history_count], input);
            history_count++;
        } else {
            /* shift history buffer to make space for new command yay */
            for (int i = 1; i < HISTORY_SIZE; i++) {
                strcpy(history[i - 1], history[i]);
            }
            strcpy(history[HISTORY_SIZE - 1], input);
        }
    }

    /* color and newline */
    terminal_setcolor(0xCC00CC);
    tprintf("\n");

    /* extract command name and arguments */
    char* command_name = strtok(input, " ");
    command_arguments = strtok(NULL, "");

    /* searching seek the command */
    int found = 0;
    for (size_t i = 0; i < command_count; i++) {
        if (strcmp(command_name, commands[i].name) == 0) {
            commands[i].function();
            found = 1;
            break;
        }
    }

    /* error :( */
    if (!found) {
        tprintf("Unknown command: ");
        tprintf(command_name);
        tprintf("\n");
    }
}

void shell_run(void) {
    char input[256];
    size_t input_len = 0;

    /* main shell loop */
    while (1) {
        /* shell prompt set variables blah blah */
        print_shell_prompt();
        input_len = 0;
        history_index = -1;

        /* read input */
        char c;
        while ((c = terminal_getchar()) != '\n') {
            if (c == '\b') { /* backspace */
                handle_backspace(input, &input_len);
            } else if (c == 0x48) { /* up arrow */
                handle_history_up(input, &input_len);
            } else if (c == 0x50) { /* down arrow */
                handle_history_down(input, &input_len);
            } else if (input_len < sizeof(input) - 1) {
                /* store char */
                input[input_len++] = c;
                terminal_putchar(c);
            }
        }
        /* process */
        process_input(input, input_len);
    }
}


