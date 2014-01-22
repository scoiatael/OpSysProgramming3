#define DEBUG
#include "common.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <langinfo.h>
#include <bsd/string.h>

char const * sperm(__mode_t mode)
{
  static char local_buff[20] = {0};
  strmode(mode, local_buff);
  /*
  int i = 0;
  // user permissions
  if ((mode & S_IRUSR) == S_IRUSR) local_buff[i] = 'r';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IWUSR) == S_IWUSR) local_buff[i] = 'w';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IXUSR) == S_IXUSR) local_buff[i] = 'x';
  else local_buff[i] = '-';
  i++;
  // group permissions
  if ((mode & S_IRGRP) == S_IRGRP) local_buff[i] = 'r';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IWGRP) == S_IWGRP) local_buff[i] = 'w';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IXGRP) == S_IXGRP) local_buff[i] = 'x';
  else local_buff[i] = '-';
  i++;
  // other permissions
  if ((mode & S_IROTH) == S_IROTH) local_buff[i] = 'r';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IWOTH) == S_IWOTH) local_buff[i] = 'w';
  else local_buff[i] = '-';
  i++;
  if ((mode & S_IXOTH) == S_IXOTH) local_buff[i] = 'x';
  else local_buff[i] = '-';*/
  return local_buff;
}

void print_info(const char* name)
{
  const char* from_slash = name;
  for (; from_slash[0] != '/'; from_slash++) {
  }
  from_slash++;
  if(*from_slash == '\0') {
    return;
  }
  struct stat buf;
  if( stat(name, &buf) == -1) {
    fatal("stat");
  }
  printf("%6d ", (int)buf.st_ino);
  printf("%4jd ", (size_t)buf.st_blocks / 2);
  printf("%10.10s", sperm(buf.st_mode));
  printf("%4d ", (int)buf.st_nlink);
  struct passwd *pwd;
  struct group *grp;
  struct tm *tm;
  char datestring[256];
  if((pwd = getpwuid(buf.st_uid)) != NULL) {
    printf("%-8.8s ", pwd->pw_name);
  } else {
    printf("%-8d ", buf.st_uid);
  }
  if((grp = getgrgid(buf.st_gid)) != NULL) {
    printf("%-8.8s ", grp->gr_name);
  } else {
    printf("%-8d ", buf.st_gid);
  }

  printf("%9jd ", (size_t)buf.st_size);
  
  tm = localtime(&buf.st_mtime);

  strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);

  printf("%s ", datestring);


  printf("%s\n", from_slash);
}

typedef struct dirn_list {
  char dirname[NAME_MAX];
  struct dirn_list *next;
} dirn_t;

void list_dir(const char* name)
{
  struct dirent *ent;
  DIR* dir;
  if((dir = opendir(name)) == NULL) {
    print_info(name);
    return;
  }
  dirn_t cdir;
  dirn_t *cur = &cdir;
  strcpy(cdir.dirname, name);
  cdir.next = NULL;
  do {
    ent = readdir(dir);
    if(ent != NULL ) {
      if((cur->next = malloc(sizeof(dirn_t))) == NULL) {
        fatal("malloc");
      }
      strcpy(cur->next->dirname, ent->d_name);
      cur->next->next = NULL;
      cur = cur->next;
    }
  } while(ent != NULL);
  cur = cdir.next;
  for (; cur != NULL;) {
    if(cur->dirname[0] != '.') {
      char* abs_name = malloc(strlen(name) + NAME_MAX + 1);
      if(abs_name == NULL) {
        fatal("malloc");
      }
      strcpy(abs_name, name);
      strcat(abs_name, "/");
      strcat(abs_name, cur->dirname);
      list_dir(abs_name);
    }
    dirn_t *temp = cur->next;
    free(cur);
    cur = temp;
  }
}

int main(int argc, char * const argv[])
{
  if( argc < 2 ) {
    fatal("Use ./5 <dir_name>" );
  }
  list_dir(argv[1]);
  return 0;
}

