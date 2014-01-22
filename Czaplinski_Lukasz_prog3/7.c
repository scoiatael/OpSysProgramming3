#define DEBUG
#include "common.h"
#include <sys/inotify.h>
#include <limits.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <utime.h>

int notify_fd = -1;
int dir_fd = -1;
void dirnotify_init(const char* dirname)
{
  if((notify_fd = inotify_init()) == -1) {
    fatal("inotify init");
  }
  if((dir_fd = inotify_add_watch(notify_fd, dirname, IN_ALL_EVENTS | IN_ONLYDIR)) == -1) {
    fatal("inotify add");
  }
}

void dirnotify_printname(const struct inotify_event* event)
{
  if(event->len > 0)
  {
    puts(event->name);
  }
}

void dirnotify_cleanup()
{
}

int from_cookie = -1;
char from_name[NAME_MAX];
int to_cookie = -1;
char to_name[NAME_MAX];


#define CASEPRINT(X) case X: \
puts("Received: " #X "");\
dirnotify_printname(event);

void dirnotify_process(const struct inotify_event* event)
{
  switch(event->mask) {
    CASEPRINT(IN_ACCESS)
      break;
    CASEPRINT(IN_ATTRIB)
      break;
    CASEPRINT(IN_CLOSE_WRITE)
      break;
    CASEPRINT(IN_CLOSE_NOWRITE)
      break;
    CASEPRINT(IN_CREATE)
      break;
    CASEPRINT(IN_DELETE)
      break;
    CASEPRINT(IN_DELETE_SELF)
      fatal("Directory deleted, exiting");
      break;
    CASEPRINT(IN_MODIFY)
      break;
    CASEPRINT(IN_MOVE_SELF)
      break;
    CASEPRINT(IN_MOVED_FROM)
      if(to_cookie > 0 && event->cookie == (unsigned)to_cookie)
      {
        printf(" ...renamed from %s\n", to_name);
        from_cookie = -1;
      } else {
        from_cookie = event->cookie;
        if(event->len > 0) {
          strcpy(from_name, event->name);
        } else {
          from_name[0] = '\0';
        }
      }
      break;
    CASEPRINT(IN_MOVED_TO)
      if(from_cookie > 0 && event->cookie == (unsigned)from_cookie)
      {
        printf(" ...renamed to %s\n", from_name);
        to_cookie = -1;
      } else {
        to_cookie = event->cookie;
        if(event->len > 0) {
          strcpy(to_name, event->name);
        } else {
          to_name[0] = '\0';
        }
      }
      break;
    CASEPRINT(IN_OPEN)
      break;
  }
}

void dirnotify_watch()
{
  char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
  while( 1 > 0) {
    read(notify_fd, buf, sizeof(buf));
    dirnotify_process((struct inotify_event*)buf);
  }
}

void sigint_handler(int sig)
{
  if(sig == SIGINT)
  {
    exit(EXIT_SUCCESS);
  }
}

void set_siginth()
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = sigint_handler;
  if(sigaction(SIGINT, &act, NULL) != 0) {
    fatal("sigaction");
  }
}

int watch(const char* name)
{
  dirnotify_init(name);
  if(atexit(dirnotify_cleanup) != 0) {
    fatal("atexit");
  }
  info("Watching dir:");
  info(name);
  dirnotify_watch();
  exit(EXIT_SUCCESS);
}

#define MODE1 (S_IXUSR | S_IRUSR | S_IWUSR)
#define MODE2 (MODE1 | S_IWGRP | S_IRGRP)
#define TEMP_NAME "OpSys7Thrash"

void close_descriptor(int* fd)
{
  if((*fd) > 0 && close(*fd) != 0) {
    fatal("close");
  }
  (*fd) = -1;
}

void open_read(const char* fullname, int* fd)
{
  assert((*fd) == -1);
  if(((*fd) = open(fullname, O_RDONLY)) < 0) {
    fatal("openr");
  }
}

void open_write(const char* fullname, int* fd)
{
  assert((*fd) == -1);
  if(((*fd) = open(fullname, O_RDWR)) < 0) {
    fatal("openw");
  }
}

void create_new_file(const char* name)
{
  if(creat(name, MODE1) < 0 && errno != EEXIST) {
    fatal("creat");
  }
  int ind = -1;
  open_write(name, &ind);
  for (int i = 0; i < 4; i++) {
    write(ind, "tt0t0t0t0tnananananananananananananananananananananan", 10);
  }
  close_descriptor(&ind);
}

void forge_new_file(char* name, const char* prefix, int* fd)
{
  int oldfd = *fd;
  close_descriptor(fd);
  char buf[NAME_MAX];
  sprintf(buf, "lala77%d5aa%ld", oldfd, (size_t) rand());
  strcpy(name, prefix);
  strcat(name, buf);
  create_new_file(name);
  
}

void hackNCreate(const char* name)
{
  struct timeb t;
  ftime(&t);
  srand( t.time + t.millitm );

  char prefix[NAME_MAX + 2], fullname[2*NAME_MAX + 2];
  strcpy(prefix, name);
  strcat(prefix, "/");
  
  int index = -1;

  forge_new_file(fullname, prefix, &index);

  struct utimbuf ut;
  memset(&ut, 0, sizeof(ut));
  struct stat buf;
  memset(&buf, 0, sizeof(buf));

  while( 1 > 0) {
    switch(rand() % 12) {
      case 3:
        printf("-------------testing close_nowrite..\n");
        /*
        close_descriptor(&index);
        open_read(fullname, &index);
        close_descriptor(&index);
        break;
        */
      case 0:
        printf("-------------testing access..\n");
  /*      if(stat(fullname, &buf) != 0) {
          fatal("stat");
        }
        ut.actime = buf.st_atim.tv_sec + 1;
        if(utime(fullname, &ut) != 0) {
          fatal("utime");
        }*/
        close_descriptor(&index);
        open_read(fullname, &index);
        char b[20];
        if(read(index, (void*)b, sizeof(b)) < 0) {
          fatal("read");
        }
        break;
      case 1:
        printf("-------------testing attrib..\n");
        if(chmod(fullname, MODE1) != 0) {
          fatal("chmod1");
        }
        if(chmod(fullname, MODE2) != 0) {
          fatal("chmod2");
        }
        break;
      case 6:
        printf("-------------testing modify..\n");
        /*
        if(stat(fullname, &buf) != 0) {
          fatal("stat");
        }
        ut.modtime = buf.st_mtim.tv_sec + 1;
        if(utime(fullname, &ut) != 0) {
          fatal("utime");
        }
        close_descriptor(&index);
        open_write(fullname, &index);
        if(write(index, "nananana", 4) < 0) {
          fatal("write");
        }
        break;
        */
      case 4:
        printf("-------------testing close_write..\n");
        close_descriptor(&index);
        open_write(fullname, &index);
        write(index, "wriiiiiiting...", 6);
        close_descriptor(&index);
        break;
      case 5:
        printf("-------------testing create..\n");
        if(unlink(fullname) != 0) {
          fatal("unlink");
        }
        forge_new_file(fullname, prefix, &index);
        break;
      case 7:
        printf("-------------testing delete..\n");
        if(unlink(fullname) != 0) {
          fatal("unlink");
        }
        forge_new_file(fullname, prefix, &index);
        break;
      case 10:
        printf("-------------testing move_to..\n");
        create_new_file(TEMP_NAME);
        if(unlink(fullname) != 0) {
          fatal("unlink");
        }
        close_descriptor(&index);
        if(rename(TEMP_NAME, fullname) != 0) {
          fatal("rename");
        }
        break;
      case 11:
        printf("-------------testing move_from..\n");
        if(rename(fullname, TEMP_NAME) != 0) {
          fatal("rename");
        }
        if(unlink(TEMP_NAME) != 0) {
          fatal("unlink");
        }
        forge_new_file(fullname, prefix, &index);
        break;
      case 8:
        printf("-------------testing delete_self..\n");
        if(unlink(fullname) != 0) {
          fatal("unlink");
        }
        close_descriptor(&index);
        if(rmdir(name) != 0) {
          fatal("rmdir");
        } else {
          fprintf(stderr, "Deleted dir, exiting..\n");
          exit(EXIT_SUCCESS);
        }
        break;
      case 9:
        printf("-------------testing move_self..\n");
        close_descriptor(&index);
        if(rename(name, "totally_not_old_directory") != 0) {
          fatal("rmdir");
        } else {
          fprintf(stderr, "Renamed old dir, exiting..\n");
          exit(EXIT_SUCCESS);
        }
        break;
    }
    sleep(1);
  }
}

pid_t pid;

void killpid()
{
  sleep(4);
  kill(pid, SIGINT);
}

int main(int argc, const char *argv[])
{
  char name[NAME_MAX+1];
  strcpy(name, "7_long_dummy_directory"); 
  if(argc > 1) {
    strncpy(name, argv[1], NAME_MAX);
    name[NAME_MAX] = '\0';
  }
  if(mkdir(name , MODE1) == -1) {
    if(errno == EEXIST) {
      printf("Using existing dir: %s\n", name);
    } else {
      fatal("mkdir");
    }
  } else {
    printf("Using newly created dir: %s\n", name);
  }
  int sib = -1;
  if((sib = fork()) == -1) {
    fatal("fork");
  }  
  switch(sib) {
    case 0:
      info("Child..");
      set_siginth();
      watch(name);
      break;
    default:
      info("Parent..");
      pid = sib;
      set_siginth();
      atexit(killpid);
      sleep(1);
      hackNCreate(name);
      break;
  }
  return 0;
}
