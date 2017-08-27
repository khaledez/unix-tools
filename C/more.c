#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LINELEN         512
#define SPACEBAR        1
#define RETURN          2
#define QUIT            3
#define INVALID         4

/**
 * do_more_of(FILE*)
 *
 * Given a FILE* argument fp, display up to a page of the
 * file fp, and then display a prompt and wait for user input.
 * If user inputs SPACEBAR, display next page.
 * If user inputs RETURN, display one more line.
 * If user inputs QUIT, terminate program.
*/
void do_more_of (FILE* filep);

/**
 * get_user_input(FILE*)
 *
 * Diplays more's status and prompt and waits for user response,
 * Requires that user press return key to receive input
 * Returns on of SPACEBAR, RETURN or QUIT on valid keypresses and
 * INVALID for invalid keypresses.
*/
int get_user_input(FILE* filep, float percent);

void get_tty_size(FILE* tty_fp, int* numrows, int* numcols);

int main (int argc, char* argv[]) {
  FILE *fp;
  int i = 0;
  if (1 == argc) {
    do_more_of (stdin);
  } else {
    while (++i < argc ) {
      fp = fopen(argv[i], "r");
      if (NULL != fp) {
        do_more_of (fp);
        fclose(fp);
      }
      else {
        printf("Skipping %s\n", argv[i]);
      }
    }
  }
  return 0;
}

void do_more_of(FILE* fp) {
  char line[LINELEN];
  int num_of_lines;
  int reply;
  int tty_rows, tty_cols;
  FILE* tty;

  // Get file size
  struct stat buffer;
  int file_size = -1;
  int chars_read = 0;
  if (0 == fstat(fileno(fp), &buffer)) { 
      file_size = buffer.st_size;
  }

  tty = fopen("/dev/tty", "r");
  if (tty == NULL)
    exit(1);

  get_tty_size(tty, &tty_rows, &tty_cols);
  num_of_lines = tty_rows;

  while (fgets(line, LINELEN, fp)) {
    chars_read += strlen(line);
    if (num_of_lines == 0) {
      // reached screen capacity so display prompt
      reply = get_user_input(tty, chars_read /(float)file_size);
      switch (reply) {
        case SPACEBAR:
          // allow full screen
          num_of_lines = tty_rows;
          printf("\033[1A\033[2K\033[1G");
          break;
        case RETURN:
          // allow one more line
          printf("\033[1A\033[2K\033[1G");
          num_of_lines++;
          break;
        case QUIT:
          printf("\033[1A\033[2K\033[1B\033[7D");
          break;
        default: // INVALID
          break;
      }
    }
    if (fputs (line, stdout) == EOF) {
      exit(1);
    }
    num_of_lines--;
  }
}

int get_user_input(FILE* fp, float percentage) {
  int c;
  int tty_rows, tty_cols;

  /**
   * Get the size of the terminal window dynamically, in case it changed.
   * Then use it to put the cursor in the bottom row, leftmost column
   * and print the prompt in "standout mode" i.e reverse video.
   */
  get_tty_size(fp, &tty_rows, &tty_cols);
  printf ("\033[%d;1H", tty_rows);
  printf ("\033[7m more? (%0.1f%%) \033[m", percentage*100);

  while ( (c = fgetc(fp)) != EOF) {
    switch (c) {
      case 'q':
        return QUIT;
      case ' ':
        return SPACEBAR;
      case '\n':
        return RETURN;
      default:
        return INVALID;
    }
  }
  return INVALID;
}

void get_tty_size(FILE* tty_fp, int* numrows, int* numcols) {
#ifdef TIOCGWINSZ
  struct winsize window_arg;
  if (-1 == ioctl(fileno(tty_fp), TIOCGWINSZ, &window_arg))
    exit(2);
  *numrows = window_arg.ws_row;
  *numcols = window_arg.ws_col;
#else
  *numrows = SCREEN_ROWS;
  *numcols = LINELEN;
#endif
}
