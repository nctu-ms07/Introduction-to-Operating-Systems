#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

static struct fuse_operations operations = {};

class Tar_Header {
 public:
  char filename[100]; //Null-terminated
  char mode[8];
  char uid[8];
  char gid[8];
  char fileSize[12];
  char lastModification[12];
  char checksum[8];
  char linkFlag; // typeFlag in Unix Standard Tar
  char linkedFileName[100];
  //Unix Standard Tar
  char magic[6]; //"ustar"
  char ustarVersion[2]; //"00"
  char ownerUserName[32];
  char ownerGroupName[32];
  char deviceMajorNumber[8];
  char deviceMinorNumber[8];
  char filenamePrefix[155];
  char padding[12];

  static uint64_t octal2decimal(char *data, size_t size) {
    uint64_t n = 0;
    for (int i = 0; i < size && isdigit(data[i]); i++) {
      n = (n << 3) | (unsigned int) (data[i] - '0');
    }
    return n;
  }

  bool isUSTAR() {
    return (memcmp(magic, "ustar", 5) == 0 && memcmp(ustarVersion, "00", 2) == 0);
  }

  mode_t getMode() {
    return octal2decimal(mode, 8);
  }

  short getUid() {
    return octal2decimal(uid, 8);
  }

  short getGid() {
    return octal2decimal(gid, 8);
  }

  size_t getFileSize() {
    return octal2decimal(fileSize, 12);
  }

  time_t getMtime() {
    return octal2decimal(lastModification, 12);
  }

  bool checkChecksum() {
    // preserve origin checksum
    char tmp[8];
    memcpy(tmp, checksum, 8);

    // set origin checksum field to spaces before calculate checksum
    memset(checksum, ' ', 8);

    // calculate both signed and unsigned checksum
    int64_t unsignedSum = 0;
    int64_t signedSum = 0;
    for (int i = 0; i < sizeof(Tar_Header); i++) {
      unsignedSum += ((unsigned char *) this)[i];
      signedSum += ((signed char *) this)[i];
    }

    //Copy back the checksum
    memcpy(checksum, tmp, 8);

    // true if checksum equals to one of the calculated checksum
    return (octal2decimal(checksum, 8) == unsignedSum || octal2decimal(checksum, 8) == signedSum);
  }
};

map<string, set<string>> file_directory;
map<string, struct stat *> file_attribute;
map<string, char *> file_content;

int do_getattr(const char *path, struct stat *st) {
  memset(st, 0, sizeof(struct stat));

  string path_s(path);

  if (path_s == "/") {
    st->st_mode = S_IFDIR | 0444;
    return 0;
  }

  if (file_attribute.count(path_s) == 1) {
    memcpy(st, file_attribute[path_s], sizeof(struct stat));
    st->st_mode = S_IFREG | st->st_mode;
    return 0;
  }

  path_s += '/';
  if (file_attribute.count(path_s) == 1) {
    memcpy(st, file_attribute[path_s], sizeof(struct stat));
    st->st_mode = S_IFDIR | st->st_mode;
    return 0;
  }

  return -ENOENT;
}

int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  string path_s(path);

  if (path_s[path_s.size() - 1] != '/') {
    path_s += '/';
  }

  filler(buffer, ".", nullptr, 0); // Current Directory
  filler(buffer, "..", nullptr, 0); // Parent Directory


  if (file_directory.count(path_s) == 1) {
    for (string file : file_directory[path_s]) {
      if (file[file.size() - 1] == '/') {
        file[file.size() - 1] = '\0';
      }
      filler(buffer, file.c_str(), nullptr, 0);
    }
  }

  return 0;
}

int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
  string path_s(path);

  if (file_content.count(path_s)) {
    size_t f_size = file_attribute[path_s]->st_size;

    if (offset >= f_size) {
      return 0;
    }

    if (offset + size > f_size) {
      memcpy(buffer, file_content[path_s] + offset, f_size - offset);
      return f_size - offset;
    }

    memcpy(buffer, file_content[path_s] + offset, size);
    return size;
  }

  return 0;
}

int main(int argc, char *argv[]) {

  fstream file;
  file.open("test.tar", ifstream::in | ifstream::binary);

  char tar_end[512];
  memset(tar_end, '\0', 512);

  while (!file.eof()) {
    Tar_Header tar_header{};
    file.read((char *) &tar_header, 512);

    if (memcmp(&tar_header, tar_end, 512) == 0) {
      break;
    }

    char *buffer = new char[tar_header.getFileSize() + 1];
    file.read(buffer, tar_header.getFileSize());
    buffer[tar_header.getFileSize()] = '\0';

    string filename(tar_header.filename);
    filename.insert(0, "/");

    smatch match;
    if (filename[filename.size() - 1] == '/') {
      regex_search(filename, match, regex("(.*/)(.*/)$"));
    } else {
      regex_search(filename, match, regex("(.*/)([^/]*)$"));
    }

    file_directory[match.str(1)].insert(match.str(2));
    struct stat *st = new struct stat;
    st->st_mode = tar_header.getMode();
    st->st_uid = tar_header.getUid();
    st->st_gid = tar_header.getGid();
    st->st_size = tar_header.getFileSize();
    st->st_mtime = tar_header.getMtime();
    st->st_nlink = 0;
    st->st_blocks = 0;

    if (file_attribute.count(filename) == 1) {
      delete[] file_attribute[filename];
      file_attribute.erase(filename);
    }

    if (file_content.count(filename) == 1) {
      delete[] file_content[filename];
      file_content.erase(filename);
    }

    file_attribute[filename] = st;
    file_content[filename] = buffer;


    // ignore paddings
    file.ignore((512 - (tar_header.getFileSize() % 512)) % 512);
  }

  memset(&operations, 0, sizeof(operations));
  operations.getattr = do_getattr;
  operations.readdir = do_readdir;
  operations.read = do_read;
  return fuse_main(argc, argv, &operations, NULL);
}