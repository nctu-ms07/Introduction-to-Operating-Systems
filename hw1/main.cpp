#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

using namespace std;

void executeCommand(vector<string> words) {
  char **args = new char *[words.size() + 1];
  for (int i = 0; i < words.size(); i++) {
    args[i] = strdup(words[i].c_str());
  }
  args[words.size()] = nullptr;
  execvp(args[0], args);
  exit(0);
}

int main() {
  signal(SIGCHLD, SIG_IGN);
  while (true) {
    cout << ">";

    string line;
    getline(cin, line);
    if (line.empty()) {
      continue;
    }

    unsigned int mode = 0;
    stringstream ss(line);
    string word;
    vector<string> words[2];
    int words_i = 0;
    while (getline(ss, word, ' ')) {
      if (word == "&") {
        mode = (int) word[0];
        continue;
      }
      if (word == ">" || word == "<" || word == "|") {
        mode = (int) word[0];
        words_i++;
        continue;
      }
      words[words_i].emplace_back(word);
    }

    pid_t child;
    switch (mode) {
      case '|':
        int pipefd[2]; //read 0, write 1
        pipe(pipefd);

        child = fork();
        if (child == 0) { //first process
          close(pipefd[0]);
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[1]);
          executeCommand(words[0]);
        } else {
          waitpid(child, nullptr, 0); // wait for first process
          child = fork();
          if (child == 0) { // second process
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            executeCommand(words[1]);
          } else { //parent process
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(child, nullptr, 0); // wait for second process
          }
        }
        break;
      case '>':
        child = fork();
        if (child == 0) { //child process
          int fd = creat(words[1].front().c_str(), 0664);
          dup2(fd, STDOUT_FILENO);
          close(fd);
          executeCommand(words[0]);
        } else { //parent process
          waitpid(child, nullptr, 0); // wait for child process
        }
        break;
      case '<':
        child = fork();
        if (child == 0) { //child process
          int fd = open(words[1].front().c_str(), O_RDONLY, 0664);
          dup2(fd, STDIN_FILENO);
          close(fd);
          executeCommand(words[0]);
        } else { //parent process
          waitpid(child, nullptr, 0); // wait for child process
        }
        break;
      case '&':
        child = fork();
        if (child == 0) { //child process
          executeCommand(words[0]);
        }
        break;
      default:
        child = fork();
        if (child == 0) { // child process
          executeCommand(words[0]);
        } else { // parent process
          waitpid(child, nullptr, 0); // wait for child process
        }
    }
  }

  return 0;
}