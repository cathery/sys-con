// This file is under the terms of the unlicense (https://github.com/DavidBuchanan314/ftpd/blob/master/LICENSE)

#pragma once

/*! Loop status */
typedef enum
{
    LOOP_CONTINUE, /*!< Continue looping */
    LOOP_RESTART,  /*!< Reinitialize */
    LOOP_EXIT,     /*!< Terminate looping */
} loop_status_t;

void network_pre_init(void);
int network_init(void);
loop_status_t network_loop(void);
void network_exit(void);
void network_post_exit(void);
