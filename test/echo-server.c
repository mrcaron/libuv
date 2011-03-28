#include "../ol.h"
#include <stdio.h>
#include <stdlib.h>


#define BUFSIZE 1024


typedef struct {
  ol_handle *handle;
  ol_req req;
  ol_buf buf;
  char read_buffer[BUFSIZE];
} peer_t;


void after_write(ol_req* req, ol_err err);
void after_read(ol_req* req, size_t nread, ol_err err);
void try_read(peer_t* peer);
void on_close(ol_handle* peer, ol_err err);
void on_accept(ol_handle* server, ol_handle* new_client);


void after_write(ol_req* req, ol_err err) {
  if (!err) {
    peer_t *peer = (peer_t*) req->data;
    try_read(peer);
  }
}


void after_read(ol_req* req, size_t nread, ol_err err) {
  if (!err) {
    if (nread == 0) {
      ol_close(req->handle);
    } else {
      peer_t *peer = (peer_t*) req->data;
      peer->buf.len = nread;
      peer->req.write_cb = after_write;
      ol_write(peer->handle, &peer->req, &peer->buf, 1);
    }
  }
}


void try_read(peer_t* peer) {
  peer->buf.len = BUFSIZE;
  peer->req.read_cb = after_read;
  ol_read(peer->handle, &peer->req, &peer->buf, 1);
}


void on_close(ol_handle* peer, ol_err err) {
  if (err) {
    fprintf(stdout, "Socket error\n");
  }

  ol_free(peer);
}


void on_accept(ol_handle* server, ol_handle* new_client) {
  new_client->close_cb = on_close;

  peer_t* p = malloc(sizeof(peer_t));
  p->handle = new_client;
  p->buf.base = p->read_buffer;
  p->buf.len = BUFSIZE;
  p->req.data = p;

  try_read(p);

  int r = ol_write2(new_client, "Hello\n");
  if (r < 0) {
    // error
    assert(0);
  }
}


int main(int argc, char** argv) {
  ol_handle* server = ol_handle_new(on_close, NULL);

  struct sockaddr_in addr = ol_ip4_addr("0.0.0.0", 8000);
  ol_bind(server, (struct sockaddr*) &addr);
  ol_listen(server, 128, on_accept);

  ol_run();

  return 0;
}
