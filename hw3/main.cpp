#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

using namespace std;

struct Arg {
  int thread_id, from, to;
};

pthread_t threads[16];
Arg args[16];
sem_t thread_start_lock[16];
sem_t thread_end_lock[16];

vector<int> single_thread_v;
vector<int> multi_thread_v;

void bubble_merge_sort(int from, int to, int level) {
  if (from < to) {

    if (level == 4) { //bubble_sort
      for (int step = 0; step < (to - from); step++) {
        for (int i = from; i < to; i++) {
          if (single_thread_v[i] > single_thread_v[i + 1]) {
            swap(single_thread_v[i], single_thread_v[i + 1]);
          }
        }
      }
      return;
    }

    //merge_sort
    int mid = (from + to) / 2;
    bubble_merge_sort(from, mid, level + 1);
    bubble_merge_sort(mid + 1, to, level + 1);

    //merge
    int left_index = 0, right_index = 0, index = from;
    vector<int> left_v(single_thread_v.begin() + from, single_thread_v.begin() + mid + 1);
    vector<int> right_v(single_thread_v.begin() + mid + 1, single_thread_v.begin() + to + 1);
    while (left_index < left_v.size() && right_index < right_v.size()) {
      if (left_v[left_index] <= right_v[right_index]) {
        single_thread_v[index++] = left_v[left_index++];
      } else {
        single_thread_v[index++] = right_v[right_index++];
      }
    }

    while (right_index < right_v.size()) {
      single_thread_v[index++] = right_v[right_index++];
    }

    while (left_index < left_v.size()) {
      single_thread_v[index++] = left_v[left_index++];
    }
  }
}

void *bubble_merge_sort(void *thread_id_p) {
  int thread_id = *(int *) thread_id_p;
  sem_wait(&thread_start_lock[thread_id]);

  if (args[thread_id].from < args[thread_id].to) {

    if (thread_id >= 8) { //bubble_sort
      for (int step = 0; step < (args[thread_id].to - args[thread_id].from); step++) {
        for (int i = args[thread_id].from; i < args[thread_id].to; i++) {
          if (multi_thread_v[i] > multi_thread_v[i + 1]) {
            swap(multi_thread_v[i], multi_thread_v[i + 1]);
          }
        }
      }
      sem_post(&thread_end_lock[thread_id]);
      return nullptr;
    }

    //merge_sort
    int mid = (args[thread_id].from + args[thread_id].to) / 2;
    int left_thread_id = thread_id * 2;
    int right_thread_id = thread_id * 2 + 1;
    args[left_thread_id].from = args[thread_id].from;
    args[left_thread_id].to = mid;
    args[right_thread_id].from = mid + 1;
    args[right_thread_id].to = args[thread_id].to;
    sem_post(&thread_start_lock[left_thread_id]);
    sem_post(&thread_start_lock[right_thread_id]);
    sem_wait(&thread_end_lock[left_thread_id]);
    sem_wait(&thread_end_lock[right_thread_id]);

    //merge
    int left_index = 0, right_index = 0, index = args[thread_id].from;
    vector<int> left_v(multi_thread_v.begin() + args[thread_id].from, multi_thread_v.begin() + mid + 1);
    vector<int> right_v(multi_thread_v.begin() + mid + 1, multi_thread_v.begin() + args[thread_id].to + 1);
    while (left_index < left_v.size() && right_index < right_v.size()) {
      if (left_v[left_index] <= right_v[right_index]) {
        multi_thread_v[index++] = left_v[left_index++];
      } else {
        multi_thread_v[index++] = right_v[right_index++];
      }
    }

    while (right_index < right_v.size()) {
      multi_thread_v[index++] = right_v[right_index++];
    }

    while (left_index < left_v.size()) {
      multi_thread_v[index++] = left_v[left_index++];
    }
  }

  sem_post(&thread_end_lock[thread_id]);
  return nullptr;
}

int main() {
  string input_name;
  cout << "Enter the input file name: ";
  cin >> input_name;
  fstream file;
  file.open(input_name, ifstream::in);
  if (file) {
    int n;
    file >> n;
    single_thread_v.resize(n);
    multi_thread_v.resize(n);
    while (n--) {
      file >> single_thread_v[n];
    }
    copy(single_thread_v.begin(), single_thread_v.end(), multi_thread_v.begin());
  }
  file.close();

  timeval start, end;
  int sec, usec;

  args[1].thread_id = 1;
  args[1].from = 0;
  args[1].to = multi_thread_v.size() - 1;
  sem_init(&thread_start_lock[1], 0, 0);
  sem_init(&thread_end_lock[1], 0, 0);
  for (int i = 2; i < 16; i++) {
    args[i].thread_id = i;
    sem_init(&thread_start_lock[i], 0, 0);
    sem_init(&thread_end_lock[i], 0, 0);
    pthread_create(&threads[i], nullptr, bubble_merge_sort, &(args[i].thread_id));
  }

  gettimeofday(&start, 0);
  sem_post(&thread_start_lock[1]);
  bubble_merge_sort(&(args[1].thread_id));
  sem_wait(&thread_end_lock[1]);
  gettimeofday(&end, 0);
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "Multi thread cost " << sec + (usec / 1000000.0f) << " sec" << endl;

  for (int i = 1; i < 16; i++) {
    sem_destroy(&thread_start_lock[i]);
    sem_destroy(&thread_end_lock[i]);
  }

  file.open("output1.txt", ofstream::trunc | fstream::out);
  if (file) {
    file << multi_thread_v[0];
    for (int i = 1; i < multi_thread_v.size(); i++) {
      file << " " << multi_thread_v[i];
    }
  }
  file.close();

  gettimeofday(&start, 0);
  bubble_merge_sort(0, single_thread_v.size() - 1, 0);
  gettimeofday(&end, 0);
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "Single thread cost " << sec + (usec / 1000000.0f) << " sec" << endl;

  file.open("output2.txt", ofstream::trunc | fstream::out);
  if (file) {
    file << single_thread_v[0];
    for (int i = 1; i < single_thread_v.size(); i++) {
      file << " " << single_thread_v[i];
    }
  }
  file.close();

  return 0;
}