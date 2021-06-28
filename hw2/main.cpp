#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/time.h>

using namespace std;

void calculate(int dim, unsigned int *input, unsigned int *output, int from, int to) {
  while (from <= to) {
    for (int x = 0; x < dim; x++) { //calculate the xth element on from row
      unsigned int sum = 0;
      for (int i = 0; i < dim; i++) {
        sum += input[from * dim + i] * input[i * dim + x];
      }
      output[from * dim + x] = sum;
    }
    from++;
  }
}

unsigned int checksum(int dim, unsigned int *matrix) {
  unsigned int sum = 0;
  for (int y = 0; y < dim; y++) {
    for (int x = 0; x < dim; x++) {
      sum += matrix[y * dim + x];
      matrix[y * dim + x] = 0;
    }
  }
  return sum;
}

int main() {
  signal(SIGCHLD, SIG_IGN);
  int dim;
  cout << "Input the matrix dimension: ";
  cin >> dim;
  size_t size = dim * dim * sizeof(unsigned int);

  int input_id = shmget(0, size, IPC_CREAT | 0660);
  int output_id = shmget(0, size, IPC_CREAT | 0660);
  unsigned int *input_matrix = (unsigned int *) shmat(input_id, NULL, 0);
  unsigned int *output_matrix = (unsigned int *) shmat(output_id, NULL, 0);

  for (int y = 0; y < dim; y++) {
    for (int x = 0; x < dim; x++) {
      input_matrix[y * dim + x] = y * dim + x;
    }
  }

  timeval start, end;

  for (int process_count = 1; process_count <= 16; process_count++) {
    cout << "Multiplying matrices using " << process_count << " process";
    if (process_count > 1) {
      cout << "es";
    }
    cout << endl;

    int processing_size = (dim / process_count);
    int module = dim % process_count;
    gettimeofday(&start, 0);
    int from = 1, to = processing_size;
    while (to <= dim) {
      if (module) {
        to++;
        module--;
      }
      if (fork() == 0) { //child process
        calculate(dim, input_matrix, output_matrix, from - 1, to - 1);
        exit(0);
      }
      from = to + 1;
      to += processing_size;
    }

    wait(nullptr);
    gettimeofday(&end, 0);
    int sec = end.tv_sec - start.tv_sec;
    int usec = end.tv_usec - start.tv_usec;
    cout << "Elapsed time: " << sec + (usec / 1000000.0f) << " sec, Checksum: " << checksum(dim, output_matrix) << endl;
  }

  shmctl(input_id, IPC_RMID, nullptr);
  shmctl(output_id, IPC_RMID, nullptr);
  shmdt(input_matrix);
  shmdt(output_matrix);
  return 0;
}