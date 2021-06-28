#include <bits/stdc++.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

using namespace std;

struct Job {
  int id, from, to;
};

Job jobs[7];

pthread_t threads[8];
sem_t job_mutex;
sem_t job_done_mutex;
queue<Job> job_queue;
sem_t job_queue_mutex;
int job_status[15]; // -1: job isn't dispatched, 0: job is dispatched, 1: job is done
sem_t job_status_mutex;

vector<int> source;
vector<int> thread_v;

void *worker_thread(void *arg) {
  while (true) {
    sem_wait(&job_mutex);
    sem_wait(&job_queue_mutex);
    Job job = job_queue.front();
    job_queue.pop();
    sem_post(&job_queue_mutex);

    if (job.from < job.to) {
      if (job.id > 6) { //bubble_sort
        for (int step = 0; step < (job.to - job.from); step++) {
          for (int i = job.from; i < job.to; i++) {
            if (thread_v[i] > thread_v[i + 1]) {
              swap(thread_v[i], thread_v[i + 1]);
            }
          }
        }
      } else { //merge
        int mid = (job.from + job.to) / 2;
        int left_index = 0, right_index = 0, index = job.from;
        vector<int> left_v(thread_v.begin() + job.from, thread_v.begin() + mid + 1);
        vector<int> right_v(thread_v.begin() + mid + 1, thread_v.begin() + job.to + 1);

        while (left_index < left_v.size() && right_index < right_v.size()) {
          if (left_v[left_index] <= right_v[right_index]) {
            thread_v[index++] = left_v[left_index++];
          } else {
            thread_v[index++] = right_v[right_index++];
          }
        }

        while (right_index < right_v.size()) {
          thread_v[index++] = right_v[right_index++];
        }

        while (left_index < left_v.size()) {
          thread_v[index++] = left_v[left_index++];
        }
      }
    }

    sem_wait(&job_status_mutex);
    job_status[job.id] = 1;
    sem_post(&job_status_mutex);
    sem_post(&job_done_mutex);
  }
}

void bubble_merge_sort(int id, int from, int to) {
  if (from < to) {
    Job job;
    job.id = id;
    job.from = from;
    job.to = to;
    if (id > 6) { //bubble_sort
      sem_wait(&job_queue_mutex);
      job_queue.emplace(job);
      sem_post(&job_queue_mutex);
      sem_post(&job_mutex);
      return;
    }
    //merge_sort
    int mid = (from + to) / 2;
    bubble_merge_sort(id * 2 + 1, from, mid);
    bubble_merge_sort(id * 2 + 2, mid + 1, to);
    jobs[job.id] = job; // store merge jobs
  }
}

int main() {
  fstream file;
  file.open("input.txt", ifstream::in);
  if (file) {
    int n;
    file >> n;
    source.resize(n);
    thread_v.resize(n);
    while (n--) {
      file >> source[n];
    }
  }
  file.close();

  timeval start, end;
  int sec, usec;

  sem_init(&job_mutex, 0, 0);
  sem_init(&job_done_mutex, 0, 0);
  sem_init(&job_queue_mutex, 0, 1);
  sem_init(&job_status_mutex, 0, 1);

  for (int i = 1; i <= 8; i++) {
    copy(source.begin(), source.end(), thread_v.begin());
    for (int &status : job_status) {
      status = -1;
    }
    pthread_create(&threads[i - 1], nullptr, worker_thread, nullptr);
    gettimeofday(&start, 0);
    bubble_merge_sort(0, 0, thread_v.size() - 1);
    while (true) {
      sem_wait(&job_done_mutex);
      sem_wait(&job_status_mutex);
      if (job_status[0] == 1) {
        sem_post(&job_status_mutex);
        break;
      }

      for (int j = 0; j <= 6; j++) {
        if (job_status[j] == -1) {
          if (job_status[j * 2 + 1] == 1 && job_status[j * 2 + 2] == 1) { // check if children is done
            job_status[j] = 0;
            sem_wait(&job_queue_mutex);
            job_queue.emplace(jobs[j]);
            sem_post(&job_queue_mutex);
            sem_post(&job_mutex);
          }
        }
      }

      sem_post(&job_status_mutex);
    }
    gettimeofday(&end, 0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;
    cout << i << " thread cost " << sec + (usec / 1000000.0f) << " sec" << endl;

    string filename = "output_" + to_string(i) + ".txt";
    file.open(filename, ofstream::trunc | fstream::out);
    if (file) {
      file << thread_v[0];
      for (int j = 1; j < thread_v.size(); j++) {
        file << " " << thread_v[j];
      }
    }
    file.close();
  }

  return 0;
}