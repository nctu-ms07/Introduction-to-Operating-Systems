#include <bits/stdc++.h>
#include <sys/time.h>

using namespace std;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return 0;
  }

  fstream file;
  unsigned int request_page;

  timeval start, end;
  int sec, usec;

  cout << "LFU policy:" << endl;
  cout << "Frame\tHit\t\tMiss\t\tPage fault ratio" << endl;
  gettimeofday(&start, 0);
  for (int frame_size = 64; frame_size <= 512; frame_size *= 2) {
    int miss = 0, hit = 0;
    map<unsigned int, list<unsigned int>, less<>> LFU_map;
    unordered_map<unsigned int, pair<unsigned int, list<unsigned int>::iterator>> LFU_hash;

    file.open(argv[1], ifstream::in);
    while (file >> request_page) {
      if (LFU_hash.count(request_page)) {
        hit++;
        int frequency = LFU_hash[request_page].first;
        LFU_map[frequency].erase(LFU_hash[request_page].second);
        if (LFU_map[frequency].empty()) {
          LFU_map.erase(frequency);
        }
        LFU_map[frequency + 1].push_front(request_page);
        LFU_hash[request_page] = make_pair(frequency + 1, LFU_map[frequency + 1].begin());
      } else {
        miss++;
        if (frame_size == LFU_hash.size()) {
          int victim_page = LFU_map.cbegin()->second.back();
          LFU_map.begin()->second.pop_back();
          if (LFU_map.cbegin()->second.empty()) {
            LFU_map.erase(LFU_map.begin());
          }
          LFU_hash.erase(victim_page);
        }
        LFU_map[1].push_front(request_page);
        LFU_hash[request_page] = make_pair(1, LFU_map[1].begin());
      }
    }
    LFU_map.clear();
    LFU_hash.clear();
    file.close();
    cout << frame_size << '\t' << hit << "\t\t" << miss << "\t\t" << fixed << setprecision(10)
         << (double) miss / (miss + hit) << endl;
  }
  gettimeofday(&end, 0);
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "Total elapsed time " << fixed << setprecision(4) <<sec + (usec / 1000000.0f) << " sec" << endl << endl;

  cout << "LRU policy:" << endl;
  cout << "Frame\tHit\t\tMiss\t\tPage fault ratio" << endl;
  gettimeofday(&start, 0);
  for (int frame_size = 64; frame_size <= 512; frame_size *= 2) {
    int miss = 0, hit = 0;
    list<unsigned int> LRU_list;
    unordered_map<unsigned int, list<unsigned int>::iterator> LRU_hash;

    file.open(argv[1], ifstream::in);
    while (file >> request_page) {
      if (LRU_hash.count(request_page)) {
        hit++;
        LRU_list.erase(LRU_hash[request_page]);
        LRU_list.push_front(request_page);
        LRU_hash[request_page] = LRU_list.begin();
      } else {
        miss++;
        if (frame_size == LRU_hash.size()) {
          LRU_hash.erase(LRU_list.back());
          LRU_list.pop_back();
        }
        LRU_list.push_front(request_page);
        LRU_hash[request_page] = LRU_list.begin();
      }
    }
    LRU_list.clear();
    LRU_hash.clear();
    file.close();
    cout << frame_size << '\t' << hit << "\t\t" << miss << "\t\t" << fixed << setprecision(10)
         << (double) miss / (miss + hit) << endl;
  }
  gettimeofday(&end, 0);
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "Total elapsed time " << fixed << setprecision(4) <<sec + (usec / 1000000.0f) << " sec" << endl;

  return 0;
}
