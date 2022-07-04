#include "csapp.h"
#include "time.h"
#include <sys/socket.h>
#include <netdb.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char * shortmsg, char *longmsg);

 


// 不断迭代，监听在命令行中传递来的端口上的连接请求。
// line:33 不断接受连接请求
// line:36 执行事务
// line:37 关闭连接
int main(int argc, char **argv)
{
    int listenfd, connfd;   // 套接字描述符， 已连接描述符 | 详情见课本p655 accept函数
    char hostname[MAXLINE], port[MAXLINE];      // 域名，端口
    socklen_t clientlen;    // sizeof(clientaddr)
    struct sockaddr_storage clientaddr;     // 客户端地址
    FILE *filelog = NULL;
    // FILE *fileini = NULL;
    // Check command-line args
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // Getaddrinfo 与 Getnameinfo，前者主机名、主机地址、服务名和端口号的字符串转化成套接字地址结构，后者则与其相反
    // fileini = fopen("www.ini", "r");

    listenfd = Open_listenfd(atoi(argv[1]));
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	    getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        filelog = fopen("log.txt", "a");
        getTime(filelog);
        fprintf(filelog, "IP: %s\t/GET\n", hostname);   // 这里咱也不知道GET在哪里存着，好像是response里回给的
        fclose(filelog);
        doit(connfd);
        Close(connfd);
    }
    // can't to close the filelog because of the continue while
}

// 获取当前时间
void getTime(FILE *fp)
{
    char len[20] = {0};
    time_t timep;
    time(&timep);
    struct tm *p;
    p = gmtime(&timep);
    fprintf(fp, "%d-%d-%d %d:%d:%d\t || \t", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec);
}

// 读取配置文件
// 还没有写
/*
*   line: 56-59 读和解析请求行
*   line: 60-63 只支持GET请求
*   line: 64    读并且忽略请求任何请求报头？
*   line: 66    将URI解析为一个文件名和一个可能为空的CGI参数字符串，并且设置一个标志，表明请求的是静态内容还是动态内容
*   如果是静态，验证由读权限 line:72，    提供静态内容 line:76 
    如果是动态，验证时可执行文件 line:80， 提供动态内容 line:84   */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
    }
    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't find this file");
            return;    
        }
        serve_static(fd, filename, sbuf.st_size);
    }

    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tint couldn't run the CGI program");
            return;
        }
    serve_dynamic(fd, filename, cgiargs);
    }
}


// 错误处理
/*
*
*/
void clienterror(int fd, char *cause, char *errnum, char * shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];
    /* Build the HTTP response body */
    // 回显信息在网页上，print中的字符串是html语法
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor = ""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tint Web server</em>\r\n", body);

    /* Print the HTTP response */
    // 回显信息在Response上，需要抓包看到详细信息，通常是给浏览器用来处理错误的
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));  
}


// TINY WEB不使用request中的报头信息，仅仅是调用read_requesthdrs函数来读取并忽略这些报头
// 终止请求报头的空文本行由回车和换行符对组成
// while循环查找\r\n
void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")){
        rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}



// TINY WEB 假设静态内容的主目录是它的当前目录，可执行文件的主目录./cgi-bin，包含cgi-bin的URI被认为是对动态内容的请求。默认的文件名为./home.html
/*
*   emmm 就是一些简单的对字符串处理
*
*
*
*
*
*/

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin")){   /* 静态 */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcpy(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }

    else {  /* 动态 */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}


// TINY WEB 提供五种静态内容HTML文件，无格式文本文件，编码为GIF,PNG,JPG格式的图片
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, srcp, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);        // 打开filename获取描述符
    srcp = Mmap(0 ,filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 将被请求文件映射到一个虚拟内存空间
    Close(srcfd);
    Rio_writen(fd, srcp, filesize); 
    Munmap(srcp, filesize);     // 释放映射的虚拟内存区域，避免潜在的内存泄露
}

/*
 * get_filetype - Derive file type from filename
 */

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "image/plain");
}



// TINY通过派生一个子进程并在进程的上下文中运行一个CGI程序来提供各类型的动态内容
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };
    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    // 派生新的子进程用来请求URI的CGI参数初始化QUERY_STRING环境变量
    if (Fork() == 0)
    {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}








