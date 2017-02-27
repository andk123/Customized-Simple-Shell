/*
 * simple_shell.h
 *
 *  Created on: 2017-02-02
 *      Author: ArnoldK
 */

#ifndef SIMPLE_SHELL_H_
#define SIMPLE_SHELL_H_

/* These two methods have to be put in the header file for both of them to know of each others
 * This is because getcmd calls getCommandHistory, which calls back getcmd after. So this would create
 * an implicit declaration for both methods without a header file.
 */
int getCommandHistory(char *args[], char *copyCmd, int *background);

int getcmd(char *line, char *args[], int *background);

//There is no other implicit declarations in the source file.

#endif /* SIMPLE_SHELL_H_ */
