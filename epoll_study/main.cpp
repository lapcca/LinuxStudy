#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <unistd.h>

#define VALID_VALUE(x,y) if(x<0)perror(y);

#define MAX_EVENTS 10
#define PORT 80

std::string getIndexContent(){
  int fd;
  struct stat fstate;
  char* path=get_current_dir_name();
  std::string fpath=path;
  free(path);
  fpath+="/index.html";
  printf("index path:%s\n",fpath.c_str());

  //open file
  if((fd =open(fpath.c_str(),O_RDONLY))<0){
  	perror("open file error\n");
  }
  //get length
  VALID_VALUE(fstat(fd,&fstate),"fstate error\n");
  
  //read file content
  size_t len=fstate.st_size;
  char* buf =new char[len];
  if (len!=read(fd,buf,len)){
  	perror("read error\n");
	return std::string("");
  }
  std::string val(buf);
  delete buf;
  return val;
}



//set socket as noblocking

void setnonblocking(int sockfd){
    int opts;

	opts = fcntl(sockfd, F_GETFL);
	if(opts < 0) {
	   perror("fcntl(F_GETFL)\n");
	   exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if(fcntl(sockfd, F_SETFL, opts) < 0) {
	    perror("fcntl(F_SETFL)\n");
	 	exit(1);
	}
}

int main(){
	struct epoll_event ev, events[MAX_EVENTS];
	int addrlen, listenfd, conn_sock, nfds, epfd, fd, i, nread, n,optval;
	struct sockaddr_in local, remote;
    char buf[BUFSIZ];
    //read index file content
	std::string idx_cnt=getIndexContent();

	//create listen socket
	if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	       perror("sockfd\n");
	       exit(1);
	}
	setnonblocking(listenfd);
	//set reuse addr flag to socket
	optval = 1;
	if(setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval))<0){
		perror("setsockopt error\n");
	}

    bzero(&local, sizeof(local));
	local.sin_family= AF_INET;
	local.sin_addr.s_addr=htonl(INADDR_ANY);
	local.sin_port=htons(PORT);
	if(bind(listenfd, (struct sockaddr*) &local, sizeof(local))<0){
		perror("bind error\n");
		exit(1);
	}
	
	listen(listenfd,20);

	epfd = epoll_create(MAX_EVENTS);
	if(epfd ==-1)
	{
		perror("epoll create error\n");
		exit(EXIT_FAILURE);
	}

	ev.events =EPOLLIN;
	ev.data.fd = listenfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd,&ev)==-1){
		perror("epoll_ctl:listen_sock\n");
		exit(EXIT_FAILURE);
	}

	while(1){
	 nfds = epoll_wait(epfd ,events,MAX_EVENTS, -1);
	 if(nfds == -1){
	 	perror("epoll_wati error\n");
		exit(EXIT_FAILURE);
	 }

	 for(i=0; i< nfds; ++i){
	 	fd= events[i].data.fd;
		if(fd == listenfd){
			while((conn_sock=accept(listenfd,(struct sockaddr*)&remote,(socklen_t *) &addrlen))>0){
				setnonblocking(conn_sock);
				ev.events =EPOLLIN |EPOLLET;
				ev.data.fd = conn_sock;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock,&ev)==-1){
					perror("epoll_ctl:listen_sock 2\n");
					exit(EXIT_FAILURE);
				}
		}
			if(conn_sock==-1){
				if(errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO &&errno != EINTR)
						perror("accept error\n");
			}
			continue;
	 	}
		
		if(events[i].events & EPOLLIN){
		  n=0;
		  while((nread= read(fd,buf+n,BUFSIZ-1))>0) {
		  	n+=nread;
		  }

		  if(nread == -1 && errno != EAGAIN){
		  	perror ("read error\n");
		  }
		  ev.data.fd= fd;
		  ev.events= events[i].events| EPOLLOUT;
		  if(epoll_ctl(epfd, EPOLL_CTL_MOD,fd,&ev)==-1){
		  	perror("epoll_ctl: mod in\n");
		  }	
		}

		if(events[i].events & EPOLLOUT){
			sprintf(buf , "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n%s",idx_cnt.length(),idx_cnt.c_str());
			int nwrite, data_size =strlen(buf);
			n= data_size;
			while(n>0){
				nwrite=write(fd,buf+data_size-n,n);
				if(nwrite<n){
					if(nwrite ==-1 && errno != EAGAIN){
						perror("write error\n");
					}
					break;
				}
				n -= nwrite;
			}
			close(fd);
		}

		}
	}
	return 0;
}

