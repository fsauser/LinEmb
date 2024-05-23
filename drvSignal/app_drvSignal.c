/***************************************************************************//**
*  \file       app_drvSignal.c
*
*  \details    Application for linux driver with signals
*
*  \author     HE-ARC 2024
*
* *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

#define REG_CURRENT_TASK _IOW('a','a',int32_t*)

#define SIGCOMM 44

static int crtlCpressed = 0;
int check = 0;

void ctrl_c_handler(int n, siginfo_t *info, void *unused)
{
  if (n == SIGINT) 
  {
    printf("Recieved ctrl-c\n");
    crtlCpressed = 1;
  }
}

void sig_event_handler(int n, siginfo_t *info, void *unused)
{
  if (n == SIGCOMM) 
  {
    check = info->si_int;
    printf ("Signal from kernel : %u\n", check);
  }
}

int main()
{
  int fd;
  int32_t num;
  struct sigaction act;

  // install ctrl-c interrupt handler
  sigemptyset (&act.sa_mask);
  act.sa_flags = (SA_SIGINFO | SA_RESETHAND);
  act.sa_sigaction = ctrl_c_handler;
  sigaction (SIGINT, &act, NULL);

  // install custom signal handler
  sigemptyset(&act.sa_mask);
  act.sa_flags = (SA_SIGINFO | SA_RESTART);
  act.sa_sigaction = sig_event_handler;
  sigaction(SIGCOMM, &act, NULL);

  printf("Signal handler for SIGCOMM = %d\n", SIGCOMM);

  printf("Opening driver\n");
  fd = open("/dev/drvSignal", O_RDWR);
  if(fd < 0) 
  {
    printf("Cannot open device file\n");
    return 0;
  }

  printf("Registering application\n");
  // register this task with kernel for signal
  if (ioctl(fd, REG_CURRENT_TASK,(int32_t*) &num)) 
  {
    printf("Failed\n");
    close(fd);
    exit(1);
  }
  printf("Done!!!\n");
 
  while(!crtlCpressed) 
  {
    printf("Waiting for signal\n");

    //blocking check
    while (!crtlCpressed && !check);
    check = 0;
  }

  printf("Closing driver\n");
  close(fd);
}

