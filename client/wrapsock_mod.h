#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
 
int
Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

void
Bind(int fd, const struct sockaddr *sa, socklen_t salen);

void
Connect(int fd, const struct sockaddr *sa, socklen_t salen);

void
Getpeername(int fd, struct sockaddr *sa, socklen_t *salenptr);

void
Getsockname(int fd, struct sockaddr *sa, socklen_t *salenptr);
void
Getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlenptr);

/* include Listen */
void
Listen(int fd, int backlog);
/* end Listen */

ssize_t
Recv(int fd, void *ptr, size_t nbytes, int flags);

ssize_t
Recvfrom(int fd, void *ptr, size_t nbytes, int flags,
		 struct sockaddr *sa, socklen_t *salenptr);

ssize_t
Recvmsg(int fd, struct msghdr *msg, int flags);

int
Select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout);
void
Send(int fd, const void *ptr, size_t nbytes, int flags);

void
Sendto(int fd, const void *ptr, size_t nbytes, int flags,
	   const struct sockaddr *sa, socklen_t salen);
void
Sendmsg(int fd, const struct msghdr *msg, int flags);

void
Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

void
Shutdown(int fd, int how);

/* include Socket */
int
Socket(int family, int type, int protocol);
/* end Socket */
