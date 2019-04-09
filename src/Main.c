#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define buffer_size 15

const char zip_command[] = "-zip";
const char unzip_command[] = "-unzip";
char zip_file_type[] = ".bzip";

char* unzip_file_type;
char buffer_output[buffer_size];
int output = 0;
int first_write = 0;

int zip(char* filename);
int unzipRLE(char* filename);
int save_buffer(char* filename, char* file_type);
char* swap_extension(char* file, char* file_type);

int main(int argc, char** argv) {
  if (argc != 3) {
    if (strcmp(argv[2], "-unzip") == 0) {
      if (argc != 4) {
        printf("Error:\tincorrect commandline arguments\n");
        printf("Format:\t./<program> <filename> <-zip/-unzip -filetype>\n");
        return -1;

      } else {
        unzip_file_type = argv[3];
      }
    } else {
      printf("Error:\tincorrect commandline arguments\n");
      printf("Format:\t./<program> <filename> <-zip>\n");
      return -1;

    }

  }

  int error = -1;
  char filename[32];
  strcpy(filename, argv[1]);

  char command[16];
  strcpy(command, argv[2]);

  if (strcmp(command, zip_command) == 0) {
    error = zip(filename);
  } else if (strcmp(command, unzip_command) == 0) {
    unzip_file_type = argv[3];
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
  } else { buffer_output[output++] = count + '0'; }

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
        // buffer_output[output++] = '\0';
        buffer_output[output++] = '\n';
        printf("Final buffer: %s\n", buffer_output);
        output = 0;
        save_buffer(filename, zip_file_type);
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
  fclose(file_input);

  return 0;
}

int save_buffer(char* filename, char* file_type) {
  FILE *file_output;

  char* dir_string = strdup(filename);
  char* file_string = strdup(filename);

  char* dir = dirname(dir_string);
  strcat(dir, "/");
  char* file_output_name = basename(file_string);

  file_output_name = swap_extension(file_output_name, file_type);
  strcat(dir, file_output_name);
  remove(dir);

  printf("%s\n", dir);

  file_output = fopen(dir, "a");
  fprintf(file_output, buffer_output);
  fclose(file_output);

  return 0;
}

char* swap_extension(char* file, char* file_type) {
  char* new_file;
  new_file = strtok(file, ".");

  strcat(new_file, file_type);

  return new_file;
}

int unzipRLE(char* filename) {
  printf("Unzipping %s...\n", filename);
  char buffer[buffer_size];

  FILE *file_input;
  char* dir_string = strdup(filename);
  char* file_string = strdup(filename);

  char* dir = dirname(dir_string);
  strcat(dir, "/");
  char* file_input_name = basename(file_string);

  file_input = fopen(filename, "r");

  while(fgets(buffer, buffer_size, file_input)) {
    printf("buffer: %s\n", buffer);
    for (int i = 0; i < buffer_size; i++) {
     buffer_output[i] = '\0';
    }
    int run_size = 0;
    char ch = 0;
    int n = 0;
    int index = 0;
    char prev_char;
    char* str = buffer;
    while (sscanf(str, "%d%c%n", &run_size, &ch, &n)) {
      str += n;
      if (ch == prev_char) {
        buffer_output[index++] = '\n';
        break;
      }

      prev_char = ch;
      printf("%d\n", n);
      printf("RUN: %d, CHAR: %c\n", run_size, ch);
      char expanded[buffer_size];
      for (int i = 0; i < run_size; i++) {
        buffer_output[index++] = ch;
      }
    }
    printf("Line : %s\n", buffer_output);
    save_buffer(filename, unzip_file_type);
  }

  fclose(file_input);

  return 0;
}
