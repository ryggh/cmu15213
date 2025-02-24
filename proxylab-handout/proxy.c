#include "csapp.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
struct socketinfo {
  char hostname[MAXLINE];
  char port[MAXLINE];
  char uri[MAXLINE];
};

struct socketinfo parse_url(char *url);
void requset_header_parser(rio_t *rp);
void doit(int fd);
int main(int argc, char **argv) {
  int listenfd, browser, serverfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  if (argc != 2) {
    fprintf(stdout, "usage: ./%s <port>\n", *argv);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    browser = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                NI_NUMERICSERV);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(browser);
    Close(browser);
  }
}
//   listenfd = Open_listenfd(argv[1]);
//   while (1) {
//     clientlen = sizeof(clientaddr);
//     connfd = Accept(listenfd, (SA *)&clientaddr,
//                     &clientlen); // line:netp:tiny:accept
//     Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port,
//     MAXLINE,
//                 NI_NUMERICSERV);
//     printf("Accepted connection from (%s, %s)\n", hostname, port);
//     doit(connfd);  // line:netp:tiny:doit
//     Close(connfd); // line:netp:tiny:close
//   }
// }
//
void doit(int fd) {
  char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE],
      errorbuf[MAXLINE], allInfo[MAXBUF], proxy_buf[MAXLINE],
      proxy_allInfo[MAXLINE];
  rio_t rio_client, rio_server;
  int proxyfd;
  struct socketinfo serverinfo;
  Rio_readinitb(&rio_client, fd);
  Rio_readlineb(&rio_client, buf, MAXLINE);
  if (errno == ECONNRESET) {
    fprintf(stderr, "Connection reset by peer (ECONNRESET). Closing socket.\n");
    return;
  }
  printf("%s", buf);
  if (!strcmp(buf, "\r\n")) {
    return;
  }
  sscanf(buf, "%s %s %s", method, url, version);
  while (strcasecmp(method, "GET")) {
    sprintf(errorbuf, "the usage: GET url version\n");
    Rio_writen(fd, errorbuf, strlen(errorbuf));
    return;
  }
  version[7] = '0';            // HTTP/1.1 -> HTTP/1.0
  serverinfo = parse_url(url); // extract the property of the url
  // printf("request server:\nhostname :%s port :%s uri :%s\n",
  //        serverinfo.hostname, serverinfo.port, serverinfo.uri);
  // connect to server
  proxyfd = Open_clientfd(serverinfo.hostname, serverinfo.port);
  Rio_readinitb(&rio_server, proxyfd);
  sprintf(allInfo, "GET %s %s\r\n", serverinfo.uri, version);
  sprintf(allInfo + strlen(allInfo), "Host: %s\r\n", serverinfo.hostname);
  sprintf(allInfo + strlen(allInfo), "%s", user_agent_hdr);
  sprintf(allInfo + strlen(allInfo), "Connection: close\r\n");
  sprintf(allInfo + strlen(allInfo), "Proxy-Connection: close\r\n");
  // concatenate client's request header and proxy's header

  Rio_readlineb(&rio_client, buf, MAXLINE);
  if (errno == ECONNRESET) {
    fprintf(stderr, "Connection reset by peer (ECONNRESET). Closing socket.\n");
    return;
  }
  while (strcmp(buf, "\r\n")) {
    if (strstr(buf, "Host") || strstr(buf, "User-Agent") ||
        strstr(buf, "Connection")) {
      Rio_readlineb(&rio_client, buf, MAXLINE);
      if (errno == ECONNRESET) {
        fprintf(stderr,
                "Connection reset by peer (ECONNRESET). Closing socket.\n");
        return;
      }
    } else {
      strcat(allInfo, buf);
      Rio_readlineb(&rio_client, buf, MAXLINE);
      if (errno == ECONNRESET) {
        fprintf(stderr,
                "Connection reset by peer (ECONNRESET). Closing socket.\n");
        return;
      }
    }
  }
  strcat(allInfo, "\r\n");

  // end concatenate
  proxy_allInfo[0] = '\0';
  printf("%s\n", allInfo);
  Rio_writen(proxyfd, allInfo, strlen(allInfo));
  // Rio_readlineb(&rio_server, proxy_buf, MAXLINE);
  // while (strcmp(proxy_buf, "\r\n")) {
  //   strcat(proxy_allInfo, proxy_buf);
  //   Rio_readlineb(&rio_server, proxy_buf, MAXLINE);
  // }
  int num;
  while ((num = Rio_readnb(&rio_server, proxy_buf, MAXLINE))) {
    if (num < 0) {
      if (errno == ECONNRESET) {
        fprintf(stderr,
                "Connection reset by peer (ECONNRESET). Closing socket.\n");
        Close(proxyfd);
        return;
      }
    }
    if (num == 0) {
      break;
    }
    Rio_writen(fd, proxy_buf, num);
  }
  Close(proxyfd);
}

struct socketinfo parse_url(char *url) {
  struct socketinfo result = {"0", "80", "0"};
  char *two_backslash = strstr(url, "//");
  char *tem = strchr(two_backslash + 2, '/');
  char *buf = strchr(two_backslash + 2, ':');
  strcpy(result.uri, tem);
  if (buf) {
    *tem = '\0';
    strcpy(result.port, buf + 1);
    *buf = '\0';
    strcpy(result.hostname, two_backslash + 2);
  } else {
    *tem = '\0';
    strcpy(result.hostname, two_backslash + 2);
    strcpy(result.port, "80");
  }
  return result;
}

void requset_header_parser(rio_t *rp) {
  char buf[MAXLINE];
  char allInfo[MAXLINE];
  while (rio_readlineb(rp, buf, MAXLINE)) {
    printf("%s", buf);
    fflush(stdout);
    if (!strcmp(buf, "\r\n")) {
      break;
    }
  }
}
