#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define buffer_size 15

const char zip_command[] = "-zip";
const char unzip_command[] = "-unzip";
const char output_file_type[] = ".bzip";

char buffer_output[buffer_size];
int output = 0;

int zipRLE(char* filename);
int zip(char* filename);
int unzipRLE(char* filename);

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Error:\tincorrect commandline arguments\n");
    printf("Format:\t./<program> <filename> <-zip/-unzip>\n");

    return -1;
  }

  int error = -1;
  char filename[32];
  strcpy(filename, argv[1]);

  char command[16];
  strcpy(command, argv[2]);

  if (strcmp(command, zip_command) == 0) {
    error = zip(filename);
  } else if (strcmp(command, unzip_command) == 0) {
    error = unzipRLE(filename);
  } else {
    printf("Error:\t%s is not a valid command.\n", command);
  }

  return 0;
}

void output_run(int count, char run) {
  printf("%d%c\n", count, run);
  if (count >= 10) {
    int num_digits = 0;
    int c = count;
    while (c != 0) {
      c = c / 10;
      num_digits++;
    }
    char num[num_digits-1];
    sprintf(num, "%d", count);
    for (int i = 0; i < num_digits; i++) {
      buffer_output[output++] = num[i];
    }
  } else buffer_output[output++] = count;

  buffer_output[output++] = run;
  printf("Buffer; %s\n", buffer_output);
}

int zip(char * filename) {
  FILE *file_input;
  char buffer[buffer_size];
  file_input = fopen(filename, "r");

  int count = 0;
  int new_char = 0;
  char run_char = ' ';

  while (fgets(buffer, buffer_size, file_input)) {
    for (int i = 0; i < buffer_size; i++) {
      buffer_output[i] = '\0';
    }
    for (int i = 0; i < buffer_size; i++) {
      if (buffer[i] == '\0') {
        buffer_output[output++] = '\n';
        printf("Final buffer: %s\n", buffer_output);
        output = 0;
        break;
      }

      if (i == 0) {
        count++;
        run_char = buffer[i];
      }
      else if (buffer[i] == buffer[i-1]) {
        if (new_char == 1) {
          new_char = 0;
        } else count++;
      }
      else if (buffer[i] != buffer[i-1]) {
        output_run(count, run_char);
        new_char = 1;
        count = 1;
        run_char = buffer[i];
      }
    }
  }

  return 0;
}

int zipRLE(char* filename) {
  printf("Zipping %s...\n", filename);

  // char* dir_string = strdup(filename);
  // char* file_string = strdup(filename);
  //
  // char* dir = dirname(dir_string);
  // char* file_input_name = basename(file_string);
  //
  // printf("%s\t%s\n", dir, file_input_name);

  FILE *file_input;
  // FILE *file_output;
  char buffer[buffer_size];

  file_input = fopen(filename, "r");
  // file_output = fopen(output_file_name, "w");

  while (fgets(buffer, buffer_size, file_input)) {
    printf("%s", buffer);
    char buffer_output[buffer_size];
    int count = 0;
    int output = 0;
    int first_index = 0;
    char run_char = ' ';
    int run = 0;

    for (int i = 0; i < buffer_size; i++) {
      buffer_output[buffer_size] = '\0';
    }

    printf("================= Next Line ===================\n");
    printf("%s\n", buffer);

    for (int i = 0; i < buffer_size; i++) {
      if (buffer[i] == '\n') continue;
      if (buffer[i] == '\0') break;
      else if (buffer[i] == buffer[i-1] || i == 0) {
        printf("Buffer i: %c\n ", buffer[i]);
        if (run == 0) {
          printf("Entering run\n");
          run = 1;
          first_index = i-1;
          run_char = buffer[i];
          printf("RUN CHAR: %c\n", run_char);
        }
        count++;
        printf("Run length: %d\n", count);
      } else if (buffer[i] != buffer[i-1]) {
        if (run == 1) {
          printf("Exiting run\n");
          run = 0;
          if (count >= 10) {
            printf("AAAAA\n");
            int num_digits = 0;
            int c = count;
            while (c != 0) {
              c = c / 10;
              num_digits++;
              printf("Num digits: %d\n", num_digits);
            }
            char num[num_digits-1];
            sprintf(num, "%d", count);
            for (int j = 0; j < num_digits; j++) {
              buffer_output[output++] = num[j];
            }
          } else {
            buffer_output[output++] = count + '0';
          }
          buffer_output[output++] = run_char;

          printf("Run char: %c\n", run_char);
          printf("After run: %s\n", buffer_output);
          count = 0;
        } else {
          printf("R: %c\n", buffer[i]);
          run_char = buffer[i];
          buffer_output[output++] = '1';
          buffer_output[output++] = run_char;
        }
      }
    }
  }

  fclose(file_input);
  // fclose(file_output);

  return 0;
}

int unzipRLE(char* filename) {
  printf("Unzipping %s...\n", filename);
  return 0;
}
