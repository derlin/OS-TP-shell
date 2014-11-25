#include <signal.h>
#include "defs.h"
#include "syserr.h"

void syserr(char *msg) {

   perror(msg);
   exit(1);
}

void fatal(char *msg) {
   fprintf(stderr, "ERROR: %s\n", msg);
   exit(1);
}

/* interpret status code */
void statusprt(int pid, int status) {
   int code;
   static char *sigmsg[] = {
      "",
      "Hangup (POSIX).",
      "Interrupt (ANSI).",
      "Quit (POSIX).",
      "Illegal instruction (ANSI).",
      "Trace trap (POSIX).",
      "Abort (ANSI).",
      "IOT trap (4.2 BSD).",
      "BUS error (4.2 BSD).",
      "Floating-point exception (ANSI).",
      "Kill, unblockable (POSIX).",
      "User-defined signal 1 (POSIX).",
      "Segmentation violation (ANSI).",
      "User-defined signal 2 (POSIX).",
      "Broken pipe (POSIX).",
      "Alarm clock (POSIX).",
      "Termination (ANSI).",
      "???",
      "Child status has changed (POSIX).",
      "Continue (POSIX).",
      "Stop, unblockable (POSIX).",
      "Keyboard stop (POSIX).",
      "Background read from tty (POSIX).",
      "Background write to tty (POSIX).",
      "Urgent condition on socket (4.2 BSD).",
      "CPU limit exceeded (4.2 BSD).",
      "File size limit exceeded (4.2 BSD).",
      "Virtual alarm clock (4.2 BSD).",
      "Profiling alarm clock (4.2 BSD).",
      "Window size change (4.3 BSD, Sun).",
      "Pollable event occurred (System V).",
      "I/O now possible (4.2 BSD).",
      "Power failure restart (System V)."
   };

   if (status != 0 && pid != 0)
      printf("Process %d: ", pid);
   if (lowbyte(status) == 0) {
      if ((code = highbyte(status)) != 0)
         printf("Exit code %d\n", code);
   }
   else {
      if ((code = status & 0177) <= NSIG)
         printf("%s", sigmsg[code]);
      else
         printf("Signal #%d", code);
      if ((status & 0200) == 0200)
         printf("-core dumped");
      printf("\n");
   }
}
