#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#define buffer_size 63
#define NUM_THREADS 2

const char zip_command[] = "-zip";
const char unzip_command[] = "-unzip";
char zip_file_type[] = ".bzip";

char* unzip_file_type;
char buffer_output[buffer_size];
char p_buffer_output[buffer_size];
int output = 0;
int first_write = 0;
int enable_pthreads = 0;
pthread_mutex_t write;
pthread_cond_t buffer_ready;

void zipRLE(char* filename);
void unzipRLE(char* filename);
void zipRLEP(char* filename);
void unzipRLEP(char* filename);
void save_buffer(char* filename, char* file_type);
char* swap_extension(char* file, char* file_type);
void* unzip(void* filename);
void* zip(void* filename);
void* p_save_buffer(void* file);
int file_size(char* filename);

// data structure for pthreads
struct file_info {
  char* filename;
  char* file_type;
  int lower_bound;
  int upper_bound;
};

int main(int argc, char** argv) {
  if (argc != 3) {
    if (strcmp(argv[2], "-unzip") == 0) {
      if (argc != 4) {
       if (strcmp(argv[4], "-p") == 0) { enable_pthreads = 1; }
       else {
          printf("Error:\tincorrect commandline arguments\n");
          printf("Format:\t./<program> <filename> <-unzip> <filetype> <-p (for pthreads)>\n");
          return -1;
       }
      } else {
        unzip_file_type = argv[3];
      }
    } else {
      if (strcmp(argv[3], "-p") == 0) { enable_pthreads = 1; }
      else {
        printf("Error:\tincorrect commandline arguments\n");
        printf("Format:\t./<program> <filename> <-zip>\n");
        return -1;
      }
    }
  }

  int error = -1;
  char filename[32];
  strcpy(filename, argv[1]);

  char command[16];
  strcpy(command, argv[2]);

  if (enable_pthreads == 0) {
    // Sequential compression method calls
    if (strcmp(command, zip_command) == 0) {
        zipRLE(filename);
    } else if (strcmp(command, unzip_command) == 0) {
      unzip_file_type = argv[3];
        unzipRLE(filename);
    } else {
      printf("Error:\t%s is not a valid command.\n", command);
    }
  } else {
    // concurrent compression method calls
    pthread_cond_init(&buffer_ready, NULL);
    pthread_mutex_init(&write, NULL);
    size_t file_length = file_size(filename);
    printf("FL : %d\n\n", (int)file_length / NUM_THREADS);
    if (strcmp(command, zip_command) == 0) {
      // 2 threads to read, 2 threads to write
      pthread_t read_thread_1 = 1;
      pthread_t read_thread_2 = 2;
      pthread_t write_thread_1 = 3;
      pthread_t write_thread_2 = 4;

      // calculate the bounds each reader thread works within
      struct file_info f = { filename, zip_file_type, 0, (int)file_length/NUM_THREADS };
      struct file_info f2 = { filename, zip_file_type, (int)file_length/NUM_THREADS, (int)file_length };

      if (pthread_create(&write_thread_1, NULL, p_save_buffer, (void*) &f)) {
        printf("Failed creating file writer thread\n");
        exit(-1);
      }
      if (pthread_create(&write_thread_2, NULL, p_save_buffer, (void*) &f)) {
        printf("Failed creating file writer thread\n");
        exit(-1);
      }
      if (pthread_create(&read_thread_1, NULL, zip, (void*) &f)) {
        printf("Failed creating file reader thread\n");
        exit(-1);
      }
      if (pthread_create(&read_thread_2, NULL, zip, (void*) &f2)) {
        printf("Failed creating file reader thread\n");
        exit(-1);
      }
      if (pthread_join(read_thread_1, NULL)) {
        printf("Failed joining file reader thread\n");
        exit(-1);
      }
      if (pthread_join(read_thread_2, NULL)) {
        printf("Failed joining file reader thread\n");
        exit(-1);
      }
      if (pthread_join(write_thread_1, NULL)) {
        printf("Failed joining file writer thread\n");
        exit(-1);
      }
      if (pthread_join(write_thread_2, NULL)) {
        printf("Failed joining file writer thread\n");
        exit(-1);
      }
    } else if (strcmp(command, unzip_command) == 0) {
      // unzipping was not implemented concurrently
      pthread_t thread_id = 1;
      pthread_create(&thread_id, NULL, unzip, (void*) filename);
      pthread_join(thread_id, NULL);
    }
  }

  return 0;
}

// used to save the run to the output buffer
void output_run(int count, char run) {
  printf("%d%c\n", count, run);
  // if the run length is more than 1 digit
  if (count >= 10) {
    int num_digits = 0;
    int c = count;
    // determine the number of digits in the run length
    while (c != 0) {
      c = c / 10;
      num_digits++;
    }
    // read the next int value from the buffer
    char num[num_digits-1];
    sprintf(num, "%d", count);
    for (int i = 0; i < num_digits; i++) {
      // multiple digit integer saving to buffer
      buffer_output[output++] = num[i];
    } // easy conversion from int to char
  } else { buffer_output[output++] = count + '0'; }

  // last char is the given character for the run
  buffer_output[output++] = run;
  printf("Buffer; %s\n", buffer_output);
}

// sequential RLE
void zipRLE(char * filename) {
  FILE *file_input;
  char buffer[buffer_size];
  file_input = fopen(filename, "r");

  int count = 0;
  int new_char = 0;
  char run_char = ' ';

  while (fgets(buffer, buffer_size, file_input)) {
    // initialize/clear buffer
    for (int i = 0; i < buffer_size; i++) {
      buffer_output[i] = '\0';
    }
    for (int i = 0; i < buffer_size; i++) {
      // condition for end of line
      if (buffer[i] == '\0') {
        // buffer_output[output++] = '\0';
        buffer_output[output++] = '\n';
        printf("Final buffer: %s\n", buffer_output);
        output = 0;

        // output the final buffer for the line
        save_buffer(filename, zip_file_type);
        break;
      }
      // the first character in the read buffer is selected immediately
      if (i == 0) {
        count++;
        run_char = buffer[i];
      }
      // calculate length of run
      else if (buffer[i] == buffer[i-1]) {
        if (new_char == 1) {
          new_char = 0;
        } else count++;
      }
      // condition for when run ends
      else if (buffer[i] != buffer[i-1]) {
        // save run to buffer
        output_run(count, run_char);
        // flag to say new character run has started
        new_char = 1;
        count = 1;
        run_char = buffer[i];
      }
    }
  }
  fclose(file_input);
}

// Writer thread method
// takes the buffer and writes it to the output file
void* p_save_buffer(void* filename) {
  pthread_mutex_lock(&write);
  // wait until the reader thread signals that the buffer is ready
  pthread_cond_wait(&buffer_ready, &write);

  struct file_info *file = (struct file_info*) filename;
  printf("%s\n%s\n", file->filename, file->file_type);
  char* dir_string = strdup(file->filename);
  char* file_string = strdup(file->filename);

  // determine the directory name
  char* dir = dirname(dir_string);
  strcat(dir, "/");

  // separate the filename from the directory
  char* file_output_name = basename(file_string);

  // change file extension to the compressed format
  file_output_name = swap_extension(file_output_name, file->file_type);
  printf("Howdy from %ld: %s\n ", pthread_self(), p_buffer_output);

  pthread_mutex_unlock(&write);
  pthread_exit(NULL);
}

void* unzip(void* filename) {
  char* file;
  file = (char*) filename;
  printf("%s", file);
}

// Reader thread method
// Same as the sequential method, but only reads the file from
// within the range specified
void* zip(void* filename) {
  char* file;
  char buffer[buffer_size];
  struct file_info *fi = (struct file_info*) filename;
  file = fi->filename;
  int count = 0;
  int new_char = 0;
  char run_char = ' ';

  FILE* f;
  printf("B");
  f = fopen(file, "r");
  printf("A");
  fseek(f, fi->upper_bound, SEEK_SET);
  printf("Bound: %d, %d\n", fi->lower_bound, fi->upper_bound);
  while(fgets(buffer, buffer_size, f)) {
    for (int i = 0; i < buffer_size; i++) {
      buffer_output[i] = '\0';
    }
    for (int i = 0; i < buffer_size; i++) {
      if (buffer[i] == '\0') {
        // buffer_output[output++] = '\0';
        buffer_output[output++] = '\n';
        printf("Final buffer: %s\n", buffer_output);
        for (int i = 0; i < buffer_size; i++) {
          p_buffer_output[i] = buffer_output[i];
        }
        if (pthread_cond_signal(&buffer_ready)) {
          printf("Error signalling thread\n");
        }
        // reset position in buffer
        output = 0;
        // save the current buffer to the output file
        save_buffer(filename, zip_file_type);
        break;
      }
      // the first character in the read buffer is selected immediately
      if (i == 0) {
        count++;
        run_char = buffer[i];
      }
      // calculate length of run
      else if (buffer[i] == buffer[i-1]) {
        if (new_char == 1) {
          new_char = 0;
        } else count++;
      }
      // condition for when run ends
      else if (buffer[i] != buffer[i-1]) {
        // save run to buffer
        output_run(count, run_char);
        // flag to say new character run has started
        new_char = 1;
        count = 1;
        run_char = buffer[i];
      }
    }
  }

  fclose(f);
  pthread_exit(NULL);
}

// Append the current buffer to the output file
void save_buffer(char* filename, char* file_type) {
  FILE *file_output;

  char* dir_string = strdup(filename);
  char* file_string = strdup(filename);

  // same as the concurrent method
  char* dir = dirname(dir_string);
  strcat(dir, "/");
  char* file_output_name = basename(file_string);

  file_output_name = swap_extension(file_output_name, file_type);
  strcat(dir, file_output_name);

  // delete the output file if it exists already
  remove(dir);

  printf("%s\n", dir);

  // open and print to the end of the file
  file_output = fopen(dir, "a");
  fprintf(file_output, buffer_output);
  fclose(file_output);
}

// determine the size of the file
// used for bounding the threads
int file_size(char* filename) {
  FILE * f;
  f = fopen(filename, "r");
  fseek(f, 0, SEEK_END);
  const int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  rewind(f);

  printf("FILE : %d\n", size);

  return size;
}


// Changes the file extension to the given type
char* swap_extension(char* file, char* file_type) {
  char* new_file;
  new_file = strtok(file, ".");

  strcat(new_file, file_type);

  return new_file;
}

// sequentially decompressed the file
void unzipRLE(char* filename) {
  printf("Unzipping %s...\n", filename);
  char buffer[buffer_size];

  FILE *file_input;
  file_input = fopen(filename, "r");

  while(fgets(buffer, buffer_size, file_input)) {
    printf("buffer: %s\n", buffer);
    // initialize/clear buffer
    for (int i = 0; i < buffer_size; i++) {
     buffer_output[i] = '\0';
    }
    int run_size = 0;
    char ch = 0;
    int n = 0;
    int index = 0;
    char prev_char;
    // copy the buffer to a string
    char* str = buffer;
    // scan the string for an int, char, and the number of characters read
    while (sscanf(str, "%d%c%n", &run_size, &ch, &n)) {
      // offset the string
      str += n;

      // end of line if the char repeats
      if (ch == prev_char) {
        buffer_output[index++] = '\n';
        break;
      }

      prev_char = ch;
      printf("%d\n", n);
      printf("RUN: %d, CHAR: %c\n", run_size, ch);

      // expand the run in the output buffer
      for (int i = 0; i < run_size; i++) {
        buffer_output[index++] = ch;
      }
    }
    printf("Line : %s\n", buffer_output);
    save_buffer(filename, unzip_file_type);
  }

  fclose(file_input);
}
