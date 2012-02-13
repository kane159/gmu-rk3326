/* 
 * Gmu Music Player
 *
 * Copyright (c) 2006-2012 Johannes Heimansberg (wejp.k.vu)
 *
 * File: httpd.c  Created: 111209
 *
 * Description: Simple (single-threaded) HTTP server (with websocket support)
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "util.h"
#include "base64.h"
#include "sha1.h"
#include "httpd.h"
#include "queue.h"

/*
 * 500 Internal Server Error
 * 501 Not Implemented
 * 400 Bad Request
 * 505 HTTP Version not supported
 */

static int send_block(int soc, unsigned char *buf, int size);
static Queue queue;
static char  webserver_root[256];

static char *http_status[] = {
	"100", "Continue",
	"101", "Switching Protocols",
	"200", "OK",
	"400", "Bad Request",
	"404", "Not Found",
	"403", "Forbidden",
	"417", "Expectation Failed",
	"500", "Internal Server Error",
	"501", "Not Implemented",
	"505", "HTTP Version not supported",
	NULL
};

static char *mime_type[] = {
	"jpg",  "image/jpeg",
	"jpeg", "image/jpeg",
	"png",  "image/png",
	"htm",  "text/html",
	"html", "text/html",
	"css",  "text/css",
	"js",   "application/x-javascript",
	"ico",  "image/vnd.microsoft.icon",
	"gz",   "application/x-gzip",
	"zip",  "application/zip",
	"txt",  "text/plain",
	NULL
};

static char *get_mime_type(char *url)
{
	char *ext, *res = "application/octet-stream";
	ext = strrchr(url, '.');
	if (ext) {
		int i;
		for (i = 0; mime_type[i]; i+=2) {
			if (strcmp(ext+1, mime_type[i]) == 0) {
				res = mime_type[i+1];
				break;
			}
		}
	}
	return res;
}

/* Returns pointer to next char after value */
static char *get_next_key_value_pair(char *str, char *key, int key_len, char *value, int value_len)
{
	int sc = 0, i = 0, eoh = 0;

	/* Check for end of header: "\n\r\n\r" */
	if ((str[sc] == '\r' && str[sc+1] == '\n') || str[sc] == '\n') {
		eoh = 1;
		printf("End of header detected!\n");
	}

	key[0] = '\0';
	value[0] = '\0';
	if (!eoh) {
		/* extract key */
		char ch = 0;
		ch = str[0];
		while (ch != '\r' && ch != '\n' && ch != ':' && ch != '\0') {
			ch = str[sc++];
			if (i < key_len-1 && ch != ':' && ch != '=' && ch != '\r' && ch != '\n') key[i++] = ch;
		}
		key[i] = '\0';
		//printf("key=[%s]\n", key);

		/* extract value */
		i = 0;
		while (str[sc] == ' ' || str[sc] == '\t') sc++; /* skip spaces after ":" */
		//if (ch != '\r' && ch != '\n') value[i++] = ch;
		ch = ' ';
		while (ch != '\r' && ch != '\n' && ch != '\0') {
			ch = str[sc++];
			if (i < value_len-1 && ch != '\r' && ch != '\n') value[i++] = ch;
		}
		value[i] = '\0';
	}
	//printf("value=[%s]\n", value);
	return str+sc+1;
}

//static int connection_counter = 0;
static Connection connection[MAX_CONNECTIONS];

static int server_running = 0;

typedef void (*sighandler_t)(int);

static sighandler_t my_signal(int sig_nr, sighandler_t signalhandler)
{
	struct sigaction neu_sig, alt_sig;

	neu_sig.sa_handler = signalhandler;
	sigemptyset(&neu_sig.sa_mask);
	neu_sig.sa_flags = SA_RESTART;
	if (sigaction(sig_nr, &neu_sig, &alt_sig) < 0)
		return SIG_ERR;
	return alt_sig.sa_handler;
}

int connection_init(Connection *c, int fd)
{
	memset(c, 0, sizeof(Connection));
	c->connection_time = time(NULL);
	c->state = HTTP_NEW;
	c->fd = fd;
	c->http_request_header = NULL;
	return 1;
}

void connection_reset_timeout(Connection *c)
{
	c->connection_time = time(NULL);
}

int connection_is_valid(Connection *c)
{
	return (c->connection_time > 0);
}

void connection_close(Connection *c)
{
	// do stuff...
	if (c->http_request_header) {
		free(c->http_request_header);
		c->http_request_header = NULL;
	}
	memset(c, 0, sizeof(Connection));
}

void connection_free_request_header(Connection *c)
{
	if (c->http_request_header) {
		free(c->http_request_header);
		c->http_request_header = NULL;
	}
}

int connection_is_timed_out(Connection *c)
{
	int timeout = c->state == WEBSOCKET_OPEN ? 
	              CONNECTION_TIMEOUT_WEBSOCKET : CONNECTION_TIMEOUT_HTTP;
	return (time(NULL) - c->connection_time > timeout);
}

void connection_set_state(Connection *c, ConnectionState s)
{
	printf("Connection: new state=%d\n", s);
	c->state = s;
}

ConnectionState connection_get_state(Connection *c)
{
	return c->state;
}

int connection_file_is_open(Connection *c)
{
	return (c->local_file ? 1 : 0);
}

int connection_file_open(Connection *c, char *filename)
{
	struct stat fst;
	if (c->local_file) fclose(c->local_file);
	c->total_size = 0;
	c->local_file = NULL;
	if (stat(filename, &fst) == 0) {
		if (S_ISREG(fst.st_mode)) {
			printf("Connection: Opening file %s (%d bytes)...\n", filename, (int)fst.st_size);
			c->local_file = fopen(filename, "r");
			c->total_size = fst.st_size;
		}
	}
	c->remaining_bytes_to_send = c->total_size;
	return (c->local_file ? 1 : 0);
}

void connection_file_close(Connection *c)
{
	if (c->local_file) {
		fclose(c->local_file);
		c->total_size = 0;
		c->local_file = NULL;
	}
}

int connection_get_number_of_bytes_to_send(Connection *c)
{
	return c->remaining_bytes_to_send;
}

int connection_file_read_chunk(Connection *c)
{
	if (c->local_file) {
		char blob[CHUNK_SIZE];
		if (c->fd && c->local_file && c->remaining_bytes_to_send > 0) {
			int size = CHUNK_SIZE;
			printf("Connection: Reading chunk of data...\n");
			if (c->remaining_bytes_to_send < CHUNK_SIZE) size = c->remaining_bytes_to_send;
			// read CHUNK_SIZE bytes from file
			fread(blob, size, 1, c->local_file);
			// write bytes to socket
			send_block(c->fd, (unsigned char *)blob, size);
			// decrement remaining bytes counter
			c->remaining_bytes_to_send -= size;
			c->connection_time = time(NULL);
		} else {
			connection_set_state(c, HTTP_IDLE);
			if (c->local_file) fclose(c->local_file);
			c->local_file = NULL;
		}
		// if eof, set connection state to HTTP_IDLE and close file, set local_file to NULL
		if (c->local_file && feof(c->local_file)) {
			fclose(c->local_file);
			c->local_file = NULL;
			connection_set_state(c, HTTP_IDLE);
		}
	} else {
		printf("Connection: ERROR, file handle invalid.\n");
	}
	return 0;
}

/* Send data of specified length 'size' to the client socket */
static int send_block(int soc, unsigned char *buf, int size)
{
	unsigned char *r = buf;
	int            len = 0;

	while (size > 0) {
		if ((len = send(soc, r, size, 0)) == -1)
			return -1;
		size -= len;
		r += len;
	}
	return 0;
}

/* Send a null-terminated string of arbitrary length to the client socket */
static int send_buf(int soc, char *buf)
{
	char *r = buf;
	int   len = 0;
	int   rlen = strlen(buf);

	if (soc) {
		while (rlen > 0) {
			if ((len = send(soc, r, strlen(r), 0)) == -1)
				return -1;
			rlen -= len;
			r += len;
		}
	}
	return 0;
}

static void send_http_header(int soc, char *code,
                             int length, time_t *pftime,
                             const char *content_type)
{
	char       msg[255] = {0};
	//struct tm *ptm = NULL, *pftm = NULL;
	//time_t     stime;
	char      *code_text = "";
	int        i;

	for (i = 0; http_status[i]; i += 2) {
		if (strcmp(http_status[i], code) == 0) {
			code_text = http_status[i+1];
			break;
		}
	}
	snprintf(msg, 254, "HTTP/1.1 %s %s\r\n", code, code_text);
	send_buf(soc, msg);
	/*stime = time(NULL);
	ptm = gmtime(&stime);
	strftime(msg, 255, "Date: %a, %d %b %Y %H:%M:%S %Z\r\n", ptm);
	send_buf(soc, msg);*/
	send_buf(soc, "Server: Gmu http server\r\n");

	/*if (pftime != NULL) {
		pftm = gmtime(pftime);
		strftime(msg, 255, "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", pftm);
		send_buf(soc, msg);
	}*/

	send_buf(soc, "Accept-Ranges: none\r\n");
	snprintf(msg, 254, "Content-Length: %d\r\n", length);
	send_buf(soc, msg);
	//send_buf(soc, "Connection: close\r\n");
	snprintf(msg, 254, "Content-Type: %s\r\n", content_type);
	send_buf(soc, msg);
	send_buf(soc, "\r\n");
}

static void loop(int listen_fd);
static int  tcp_server_init(int port);

void *httpd_run_server(void *data)
{
	int   port = SERVER_PORT;
	char *wr = (char *)data;

	if (wr) strncpy(webserver_root, wr, 255); else webserver_root[0] = '\0';
	queue_init(&queue);
	printf("Starting server on port %d.\n", port);
	loop(tcp_server_init(port));
	return NULL;
}

#define MAXLEN 4096

#define OKAY 0
#define ERROR (-1)

/*
 * Open server listen port 'port'
 * Returns socket fd
 */
static int tcp_server_init(int port)
{
	int                listen_fd, ret, yes = 1;
	struct sockaddr_in sock;

	listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	if (listen_fd >= 0) {
		/* Avoid "Address already in use" error: */
		ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (ret >= 0) {
			memset((char *) &sock, 0, sizeof(sock));
			sock.sin_family = AF_INET;
			sock.sin_addr.s_addr = htonl(INADDR_ANY);
			sock.sin_port = htons(port);

			ret = bind(listen_fd, (struct sockaddr *) &sock, sizeof(sock));
			if (ret == 0) {
				ret = listen(listen_fd, 5);
				if (ret < 0) listen_fd = ERROR;
			} else {
				listen_fd = ERROR;
			}
		} else {
			listen_fd = ERROR;
		}
	} else {
		listen_fd = ERROR;
	}
	return listen_fd;
}

/*
 * Open connection to client. To be called for every new client.
 * Returns socket fs when successful, ERROR otherwise
 */
static int tcp_server_client_init(int listen_fd)
{
	int                fd;
	struct sockaddr_in sock;
	socklen_t          socklen;

	socklen = sizeof(sock);
	fd = accept(listen_fd, (struct sockaddr *) &sock, &socklen);

	return fd < 0 ? ERROR : fd;
}

/*
 * Write to client socket
 * Returns OKAY when the message could be written completely, ERROR otherwise
 */
static int tcp_server_write(int fd, char buf[], int buflen)
{
	return write(fd, buf, buflen) == buflen ? OKAY : ERROR;
}

/*
 * Read from client socket
 * Returns OKAY when reading was successful, ERROR otherwise
 */
static int tcp_server_read(int fd, char buf[], int *buflen)
{
	int res = OKAY;
	*buflen = read(fd, buf, *buflen);
	if (*buflen <= 0) { /* End of TCP connection */
		close(fd);
		res = ERROR; /* fd no longer valid */
	}
	return res;
}

static int is_valid_ressource(char *str)
{
	int res = 1, i, len = 0;
	/* Ressource strings MUST NOT contain substrings containing ".." */
	if (str) {
		len = strlen(str);
		for (i = 0; i < len; i++) {
			if (str[i] == '.') {
				if (i+1 < len && str[i+1] == '.') {
					res = 0;
					break;
				}
			}
		}
	}
	return res;
}

static HTTPCommand get_command(char *str)
{
	HTTPCommand cmd = UNKNOWN;
	if (str) {
		if      (strcmp(str, "GET")  == 0) cmd = GET;
		else if (strcmp(str, "HEAD") == 0) cmd = HEAD;
		else if (strcmp(str, "POST") == 0) cmd = POST;
	}
	return cmd;
}

static void http_response_bad_request(int fd, int head_only)
{
	char *str = "<h1>400 Bad Request</h1>";
	int   body_len = strlen(str);
	send_http_header(fd, "400", body_len, NULL, "text/html");
	if (!head_only) tcp_server_write(fd, str, body_len);
}

static void http_response_not_found(int fd, int head_only)
{
	char *str = "<h1>404 File not found</h1>";
	int   body_len = strlen(str);
	send_http_header(fd, "404", body_len, NULL, "text/html");
	if (!head_only) tcp_server_write(fd, str, body_len);
}

static void http_response_not_implemented(int fd)
{
		char *str =
				"<html><head><title>501 Not implemented</title></head>\n"
				"<body>\n<h1>501 Not implemented</h1>\n<p>Invalid method.</p>\n"
				"<p><hr /><i>Gmu http server</i></p>\n</body>\n"
				"</html>\r\n\r\n";
		int   body_len = strlen(str);
		send_http_header(fd, "501", body_len, NULL, "text/html");
		tcp_server_write(fd, str, body_len);
}

static void websocket_send_string(Connection *c, char *str)
{	
	if (str) {
		char *buf;
		int   len = strlen(str);
		
		buf = malloc(len+10); /* data length + 1 byte for flags + 9 bytes for length (1+8) */
		if (buf) {
			memset(buf, 0, len);
			printf("websocket_send_string(): len=%d str='%s' %d|%d\n", len, str, len >> 8, len & 0xFF);
			if (len <= 125) {
				snprintf(buf, 1023, "%c%c%s", 0x80+0x01, len, str);
				send_block(c->fd, (unsigned char *)buf, len+2);
			} else if (len > 125 && len < 65535) {
				snprintf(buf, 1023, "%c%c%c%c%s", 0x80+0x01, 126, 
						 (unsigned char)(len >> 8), (unsigned char)(len & 0xFF), str);
				send_block(c->fd, (unsigned char *)buf, len+4);
			} else { /* More than 64k bytes */
				/* Such long strings are not supported right now */
			}
			free(buf);
		}
		connection_reset_timeout(c);
	}
}

static char *websocket_unmask_message_alloc(char *msgbuf, int msgbuf_size)
{
	char *flags, *mask = NULL, *message, *unmasked_message = NULL;
	int   masked, len, i, offset = 0;

	flags  = msgbuf;
	len    = msgbuf[1] & 127;
	if (len == 126) {
		len = (((unsigned int)msgbuf[2]) << 8) + (unsigned char)msgbuf[3];
		offset += 2;
	} else if (len == 127) {
		len = -1; /* unsupported */
	}

	if (len > 0 && len < msgbuf_size) {
		masked = (msgbuf[1] & 128) ? 1 : 0;
		if (masked) mask = msgbuf + 2 + offset;
		message = msgbuf + 2 + (masked ? 4 : 0) + offset;
		printf("websocket_unmask_message_alloc(): len=%d flags=%x masked=%d\n", len, flags[0], masked);
		if (masked) {
			unmasked_message = malloc(len+1);
			if (unmasked_message)  {
				for (i = 0; i < len; i++) {
					char c = message[i] ^ mask[i % 4];
					unmasked_message[i] = c;
				}
				unmasked_message[len] = '\0';
				printf("websocket_unmask_message_alloc(): unmasked text='%s'\n", unmasked_message);
			}
		} else {
			printf("websocket_unmask_message_alloc(): ERROR: Unmasked message: [%s]\n", message);
		}
	} else {
		printf("websocket_unmask_message_alloc(): invalid length: %d (max. length: %d)\n", len, msgbuf_size);
	}
	return unmasked_message;
}

static int process_command(int rfd, Connection *c)
{
	char *command = NULL, *ressource = NULL, *http_version = NULL, *options = NULL;
	char *host = NULL, websocket_key[32] = "";

	if (c->http_request_header) {
		int   len = strlen(c->http_request_header);
		int   i, state = 1;
		char *str;
		
		str = malloc(len+1);
		memset(str, 0, len); // Huh?
		if (str) {
			strncpy(str, c->http_request_header, len);
			str[len] = '\0';
			command = str; /* GET, POST, HEAD ... */

			/* Parse HTTP request (first line)... */
			for (i = 0; i < len && str[i] != '\r' && str[i] != '\n'; i++) {
				if (str[i] == ' ' || str[i] == '\t') {
					str[i] = '\0'; i++;
					for (; str[i] == ' ' || str[i] == '\t'; i++);
					switch (state) {
						case 1: /* ressource */
							ressource = str+i;
							break;
						case 2: /* http version */
							http_version = str+i; /* e.g. "HTTP/1.1" */
							break;
					}
					state++;
				}
			}
			str[i] = '\0';
			/* Parse options (following lines)... */
			if (i < len) {
				options = str+i+1;
				if (options) {
					len = strlen(options);
					for (; options[0] != '\0' && (options[0] == '\r' || options[0] == '\n'); options++);
				} else {
					printf("No options. :(\n");
				}
			} else {
				printf("Unexpected end of data.\n");
			}
			for (i = len-1; i > 0 && (str[i] == '\n' || str[i] == '\r'); i--)
				str[i] = '\0';
		}
		if (command) printf("%04d Command: [%s]\n", rfd, command);
		if (ressource) {
			printf("%04d Ressource: [%s]\n", rfd, ressource);
			printf("%04d Mime type: %s\n", rfd, get_mime_type(ressource));
		}
		if (http_version) printf("%04d http_version: [%s]\n", rfd, http_version);
		if (options) {
			char key[128], value[256], *opts = options;
			int  websocket_version = 0, websocket_upgrade = 0, websocket_connection = 0;
			printf("%04d Options: [%s]\n", rfd, options);
			while (opts) {
				opts = get_next_key_value_pair(opts, key, 128, value, 256);
				if (key[0]) {
					printf("key=[%s] value=[%s]\n", key, value);
					if (strcasecmp(key, "Host") == 0) host = value; // not good, we need to copy the string!
					if (strcasecmp(key, "Upgrade") == 0 && strcasecmp(value, "websocket") == 0)
						websocket_upgrade = 1;
					if (strcasecmp(key, "Connection") == 0 && strcasecmp(value, "Upgrade") == 0)
						websocket_connection = 1;
					if (strcasecmp(key, "Sec-WebSocket-Key") == 0) {
						strncpy(websocket_key, value, 31);
						websocket_key[31] = '\0';
					}
					if (strcasecmp(key, "Sec-WebSocket-Version") == 0 && value[0]) 
						websocket_version = atoi(value);
				}
				if (!(key[0])) break;
			}
			if (websocket_upgrade && websocket_connection && websocket_version && websocket_key[0] &&
			    get_command(command) == GET) {
				printf("---> Client wants to upgrade connection to WebSocket version %d with key '%s'.\n",
				       websocket_version, websocket_key);
			} else { /* If there is no valid websocket request, clear the key as it being used later
			            to check for websocket request */
				websocket_key[0] = '\0';
			}
		}

		if (is_valid_ressource(ressource)) {
			int head_only = 0;
			switch (get_command(command)) {
				case HEAD:
					head_only = 1;
				case GET: {
					int file_okay = 0;
					if (host) { /* If no Host has been supplied, the query is invalid, thus repond with 400 */
						if (websocket_key[0]) { /* Websocket connection upgrade */
							char *websocket_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
							char str[61], str2[61];
							SHA1_CTX sha;
							uint8_t  digest[SHA1_DIGEST_SIZE];
							printf("%04d Proceeding with websocket connection upgrade...\n", rfd);
							/* 1) Calculate response key (SHA1, Base64) */
							snprintf(str, 61, "%s%s", websocket_key, websocket_magic);
							str[60] = '\0';
							printf("Voodoo = '%s'\n", str);
							SHA1_Init(&sha);
							SHA1_Update(&sha, (const uint8_t *)str, strlen(str));
							SHA1_Final(&sha, digest);
							memset(str, 0, 61);
							base64_encode_data((unsigned char *)digest, 20, str, 30);
							printf("Calculated base 64 response value: '%s'\n", str);
							/* 2) Send 101 response with appropriate data */
							send_buf(rfd, "HTTP/1.1 101 Switching Protocols\r\n");
							send_buf(rfd, "Server: Gmu http server\r\n");
							send_buf(rfd, "Upgrade: websocket\r\n");
							send_buf(rfd, "Connection: Upgrade\r\n");
							snprintf(str2, 60, "Sec-WebSocket-Accept: %s\r\n", str);
							printf(str2);
							send_buf(rfd, str2);
							send_buf(rfd, "\r\n");
							/* 3) Set flags in connection struct to WebSocket */
							connection_set_state(c, WEBSOCKET_OPEN);
							websocket_send_string(c, "{ \"cmd\": \"hello\" }");
						} else if (!connection_file_is_open(c)) { // ?? open file (if not open already) ??
							char filename[512];
							memset(filename, 0, 512);
							snprintf(filename, 511, "%s/htdocs%s", webserver_root, ressource);
							file_okay = connection_file_open(c, filename);
							if (file_okay) {
								if (!head_only) connection_set_state(c, HTTP_BUSY);
								send_http_header(rfd, "200",
												 connection_get_number_of_bytes_to_send(c),
												 NULL,
												 get_mime_type(ressource));
								if (head_only) connection_file_close(c);
							} else { /* 404 */
								http_response_not_found(rfd, head_only);
							}
						}
					} else {
						http_response_bad_request(rfd, head_only);
					}
					break;
				}
				case POST:
					http_response_not_implemented(rfd);
					break;
				default:
					http_response_bad_request(rfd, 0);
					break;
			}
		} else { /* Invalid ressource string */
			http_response_not_found(rfd, 0);
		}

		if (str) free(str);
		connection_free_request_header(c);
	}
	return 0;
}

/*
 * Send string "str" to all connected WebSocket clients
 */
void httpd_send_websocket_broadcast(char *str)
{
	queue_push(&queue, str);
}

/* 
 * Webserver main loop
 * listen_fd is the socket file descriptor where the server is 
 * listening for client connections.
 */
static void loop(int listen_fd)
{
	fd_set the_state;
	int    maxfd;

	FD_ZERO(&the_state);
	FD_SET(listen_fd, &the_state);
	maxfd = listen_fd;

	my_signal(SIGPIPE, SIG_IGN);

	server_running = 1;
	while (server_running) {
		fd_set         readfds;
		int            ret, rfd;
		struct timeval tv;
		char          *websocket_msg;

		/* previously: 2 seconds timeout */
		tv.tv_sec  = 0;
		tv.tv_usec = 1000;

		readfds = the_state; /* select() aendert readfds */
		ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
		if ((ret == -1) && (errno == EINTR)) {
			/* A signal has occured; ignore it. */
			continue;
		}
		if (ret < 0) break;

		/* Check TCP listen port for incoming connections... */
		if (FD_ISSET(listen_fd, &readfds)) {
			rfd = tcp_server_client_init(listen_fd);
			if (rfd >= 0) {
				FD_SET(rfd, &the_state); /* add new client */
				if (rfd > maxfd) maxfd = rfd;
				//connection_counter++;
				connection_init(&(connection[rfd-listen_fd-1]), rfd);
				printf("Connection count: ++\n");
			}
		}

		/* Check WebSocket data send queue and fetch the next item if at
		 * least one is available */
		websocket_msg = queue_pop_alloc(&queue);

		/* Check all open TCP connections for incoming data. 
		 * Also handle ongoing http file requests and pending 
		 * WebSocket transmit requests here. */
		for (rfd = listen_fd + 1; rfd <= maxfd; ++rfd) {
			int conn_num = rfd-listen_fd-1;
			if (connection_get_state(&(connection[conn_num])) == HTTP_BUSY) { /* feed data */
				/* Read CHUNK_SIZE bytes from file and send data to socket & update remaining bytes counter */
				connection_file_read_chunk(&(connection[conn_num]));
			} else if (connection[conn_num].state == WEBSOCKET_OPEN) {
				char str[256];
				static int i = 0, t = 0;
				/* If data for sending through websocket have been fetched
				 * from the queue, send the data to all open WebSocket connections */
				if (websocket_msg) {
					websocket_send_string(&(connection[conn_num]), websocket_msg);
				}
				if (i == 1000) {
					snprintf(str, 255, "{ \"cmd\": \"time\", \"time\" : %d }", t++);
					websocket_send_string(&(connection[conn_num]), str);
					i = 0;
				}
				i++;
			}
			if (FD_ISSET(rfd, &readfds)) { /* Data received on connection socket */
				char msgbuf[MAXLEN+1];
				int  msgbuflen, request_header_complete = 0;

				/* Read message from client */
				msgbuf[MAXLEN] = '\0';
				msgbuflen = sizeof(msgbuf);
				//memset(msgbuf, 0, MAXLEN);
				ret = tcp_server_read(rfd, msgbuf, &msgbuflen);
				if (ret == ERROR) {
					FD_CLR(rfd, &the_state); /* remove dead client */
					//connection_counter--;
					printf("Connection count: --\n");
				} else {
					int len = msgbuflen;
					int len_header = 0;

					if (connection[conn_num].state != WEBSOCKET_OPEN) {
						printf("%04d http message.\n", rfd);
						if (connection[conn_num].http_request_header)
							len_header = strlen(connection[conn_num].http_request_header);
						connection[conn_num].http_request_header = 
							realloc(connection[conn_num].http_request_header, len_header+len+1);
						if (connection[conn_num].http_request_header) {
							memcpy(connection[conn_num].http_request_header+len_header, msgbuf, len);
							connection[conn_num].http_request_header[len_header+len] = '\0';
						}
						if (strstr(connection[conn_num].http_request_header, "\r\n\r\n") || 
							strstr(connection[conn_num].http_request_header, "\n\n")) { /* we got a complete header */
							request_header_complete = 1;
						}
					} else {
						char *unmasked_message = NULL;
						printf("%04d websocket message received.\n", rfd);
						unmasked_message = websocket_unmask_message_alloc(msgbuf, MAXLEN);
						connection_reset_timeout(&(connection[conn_num]));

						if (unmasked_message) {
							char str[1000];
							snprintf(str, 999, "{ \"cmd\": \"echo\", \"message\" : \"%s\" }", unmasked_message);
							websocket_send_string(&(connection[conn_num]), str);
						}
						if (unmasked_message) free(unmasked_message);
					}
				}
				if (request_header_complete)
					process_command(rfd, &(connection[conn_num]));
			} else { /* no data received */
				/* Just testing ... */
				//if (connection_is_valid(&(connection[conn_num])) && connection[conn_num].state == WEBSOCKET_OPEN)
					//websocket_send_string(&(connection[conn_num]), "baz");
	
				if (connection_is_valid(&(connection[conn_num])) &&
					connection_is_timed_out(&(connection[conn_num]))) { /* close idle connection */
					printf("Closing connection to idle client %d...\n", rfd);
					FD_CLR(rfd, &the_state);
					close(rfd);
					connection_close(&(connection[conn_num]));
					//connection_counter--;
					printf("Connection count: -- (idle)\n");
				}
			}
		}
		if (websocket_msg) free(websocket_msg);
	}
	queue_free(&queue);
}

void httpd_stop_server(void)
{
	server_running = 0;
}
