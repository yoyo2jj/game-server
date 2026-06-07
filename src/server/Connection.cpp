#include "server/Connection.h"

Connection::Connection(int fd)
	:fd_(fd),isLoggedIn_(false),roomId_(-1){}
