#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <glob.h>
#include <signal.h>

/* Starts mongrel2 and sends various stuff at it */


#define ensure(msg, test) do { if(test < 0) { perror(msg); return 0; } } while(0)

char buf[32768];
int bufstart = 0;

int do_test(char *server, char *port, char *req) {
  int s;  
  struct sockaddr_in sa;  
  int done = 0;
  int nread;
  struct pollfd pfd[1];
  int timeout = 5;
  int countdown;
  char *clen_hdr = "Content-Length: ";
  int clen;
  char *pc;
  int body_idx;

  memset(&sa, 0, sizeof(sa));

  ensure("Socket", (s = socket(AF_INET, SOCK_STREAM, 0)));
  ensure("inet_aton", inet_aton(server, &sa.sin_addr));

  sa.sin_family = AF_INET;     
  sa.sin_port = htons(atoi(port));   

  ensure("Connect", connect(s, (struct sockaddr *)&sa, sizeof(struct sockaddr)));
  ensure("Send Request", send(s, req, strlen(req), 0) - strlen(req));
  bufstart = 0; 
  buf[bufstart] = 0;


  while(!done && (sizeof(buf)-bufstart > 1))  {
    pfd[0].fd = s;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    countdown = timeout;
    while(!pfd[0].revents & POLLIN) {
      poll(pfd, 1, 1000);
      ensure("Poll for Reply headers - timeout", countdown--);
    }
    ensure("Recv Reply", (nread = recv(s, &buf[bufstart], sizeof(buf)-bufstart-1, 0)));
    buf[bufstart + nread] = 0;
    done = (NULL != strstr(buf, "\r\n\r\n"));
    bufstart += nread;
  }
  ensure("Reading headers - buffer overflow", sizeof(buf)-bufstart);
  ensure("Header reading not done", done-1);
  ensure("Finding Content-Length", (pc = strstr(buf, clen_hdr)) - buf);
  pc += strlen(clen_hdr);
  ensure("Converting content-length", (clen = atoi(pc))-1);
  body_idx = (strstr(buf, "\r\n\r\n")-buf)+4;
  clen -= (bufstart - body_idx);
  done = 0;
  
  while(!done && (clen > 0) && (sizeof(buf)-bufstart > 1))  {
    pfd[0].fd = s;
    pfd[0].events = POLLIN;
    pfd[0].revents = 0;
    countdown = timeout;
    while(!pfd[0].revents & POLLIN) {
      poll(pfd, 1, 1000);
      ensure("Poll for Reply body - timeout", countdown--);
    }
    ensure("Recv Reply (body)", (nread = recv(s, &buf[bufstart], sizeof(buf)-bufstart-1, 0)));
    buf[bufstart + nread] = 0;
    bufstart += nread;
    clen -= nread;
  }
  ensure("Reading body - buffer overflow", sizeof(buf)-bufstart);
  ensure("Content-Length check failure", -clen);
  
  return 1;
}



int run_tests(char *testdir, char *server, char *port) {
  glob_t test_files;
  int nrun = 0;
  int errors = 0;
  int i;
  FILE *manifest_file;
  char str[512];
  char fname[512];
  int rc;
  int request_complete = 1;
  snprintf(str, sizeof(str)-1, "%s/*", testdir);
  rc = glob(str, 0, NULL, &test_files);
  for(i = 0; i < test_files.gl_pathc; i++) {
    snprintf(buf, sizeof(buf)-1, "%s/request-manifest", test_files.gl_pathv[i]);
    manifest_file = fopen(buf, "r"); 
    if (manifest_file) {
      // printf("Preparing test %s\n", test_files.gl_pathv[i]);
      bufstart = 0;
      while(fgets(str, sizeof(str)-1, manifest_file)) {
        FILE *datafile;
        int nread;
	while((strlen(str) > 0) && isspace(str[strlen(str)-1])) { str[strlen(str)-1] = 0; }
        snprintf(fname, sizeof(fname)-1, "%s/%s", test_files.gl_pathv[i], str);
        if(datafile = fopen(fname, "r")) {
          struct stat st;
          stat(fname, &st);
          nread = fread(&buf[bufstart], 1, sizeof(buf)-1-bufstart, datafile);
          fclose(datafile);
          if(nread == st.st_size) {
            bufstart += nread;
            buf[bufstart] = 0;
          } else {
            printf("Could not read from %s: too much to fit in our buffers\n", str);
            request_complete = 0;
          }
        } else {
          printf("Could not open %s\n", fname);
          request_complete = 0;
        }
      }
      if(request_complete) {
        nrun++;
        if(do_test(server, port, buf)) {
          printf("Reply for test %s:\n%s\n", test_files.gl_pathv[i], buf);
        } else {
	  errors++;
          printf("Our test %s failed during execution\n", test_files.gl_pathv[i]);
        }
      } else {
        printf("Our test %s was not run: could not assemble request\n", test_files.gl_pathv[i]);
      }
      
    } else {
      printf("NOT preparing test %s, could not open %s\n", test_files.gl_pathv[i], buf);
    }
  }
  printf("Tests run: %d, errors: %d\n", nrun, errors);
  return errors;
}

int launch_m2(char *m2path, char *dir, char *uuid) {
  int pid;
  char *argv[3] = { m2path, "config.sqlite", uuid };
  if(pid = fork()) {
    return pid;
  } else {
    chdir(dir);
    execv(m2path, argv);
  }
}

int main(int argc, char *argv[]) {
  if(argc < 8) {
    printf("Usage: %s <mongrel2-path> <site-dir> <server-uuid> <sleep-seconds-before-tests> <testdir> <server_addr> <server_port>\n", argv[0]);
    exit(1);
  }
  int cpid = launch_m2(argv[1], argv[2], argv[3]);
  sleep(atoi(argv[4]));
  int errors = run_tests(argv[5], argv[6], argv[7]);
  kill(cpid, SIGINT);
  exit(errors);
}




