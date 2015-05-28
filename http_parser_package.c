#include "http_parser.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <semaphore.h>
#include "http_parser_package.h"

static sem_t sem;

static int currently_parsing_eof;
static struct message *messages;
static int num_messages;
static http_parser request_parser, response_parser;

/* strnlen() is a POSIX.2008 addition. Can't rely on it being available so
 * define it ourselves.
 */
size_t
strnlen(const char *s, size_t maxlen)
{
  const char *p;

  p = memchr(s, '\0', maxlen);
  if (p == NULL)
    return maxlen;

  return p - s;
}

size_t
strlncat(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t dlen;
  size_t rlen;
  size_t ncpy;

  slen = strnlen(src, n);
  dlen = strnlen(dst, len);

  if (dlen < len) {
    rlen = len - dlen;
    ncpy = slen < rlen ? slen : (rlen - 1);
    memcpy(dst + dlen, src, ncpy);
    dst[dlen + ncpy] = '\0';
  }

  assert(len > slen + dlen);
  return slen + dlen;
}

size_t
strlcat(char *dst, const char *src, size_t len)
{
  return strlncat(dst, len, src, (size_t) -1);
}

size_t
strlncpy(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t ncpy;

  slen = strnlen(src, n);

  if (len > 0) {
    ncpy = slen < len ? slen : (len - 1);
    memcpy(dst, src, ncpy);
    dst[ncpy] = '\0';
  }

  assert(len > slen);
  return slen;
}

size_t
strlcpy(char *dst, const char *src, size_t len)
{
  return strlncpy(dst, len, src, (size_t) -1);
}

int
request_url_cb (http_parser *p, const char *buf, size_t len)
{
  assert(p);
  strlncat(messages[num_messages].request_url,
           sizeof(messages[num_messages].request_url),
           buf,
           len);

  return 0;
}

int
header_field_cb (http_parser *p, const char *buf, size_t len)
{
  assert(p);
  struct message *m = &messages[num_messages];

  if (m->last_header_element != FIELD)
    m->num_headers++;

  strlncat(m->headers[m->num_headers-1][0],
           sizeof(m->headers[m->num_headers-1][0]),
           buf,
           len);

  m->last_header_element = FIELD;

  return 0;
}

int
header_value_cb (http_parser *p, const char *buf, size_t len)
{
  assert(p);
  struct message *m = &messages[num_messages];

  strlncat(m->headers[m->num_headers-1][1],
           sizeof(m->headers[m->num_headers-1][1]),
           buf,
           len);

  m->last_header_element = VALUE;

  return 0;
}

void
check_body_is_final (const http_parser *p)
{
  if (messages[num_messages].body_is_final) {
    fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
                    "on last on_body callback call "
                    "but it doesn't! ***\n\n");
    assert(0);
    abort();
  }
  messages[num_messages].body_is_final = http_body_is_final(p);
}

int
body_cb (http_parser *p, const char *buf, size_t len)
{
  assert(p);
  strlncat(messages[num_messages].body,
           sizeof(messages[num_messages].body),
           buf,
           len);
  messages[num_messages].body_size += len;
  check_body_is_final(p);
 // printf("body_cb: '%s'\n", requests[num_messages].body);
  return 0;
}

int
message_begin_cb (http_parser *p)
{
  assert(p);
  messages[num_messages].message_begin_cb_called = TRUE;
  return 0;
}

int
headers_complete_cb (http_parser *p)
{
  assert(p);
  messages[num_messages].method = p->method;
  messages[num_messages].status_code = p->status_code;
  messages[num_messages].http_major = p->http_major;
  messages[num_messages].http_minor = p->http_minor;
  messages[num_messages].headers_complete_cb_called = TRUE;
  messages[num_messages].should_keep_alive = http_should_keep_alive(p);
  return 0;
}

int
message_complete_cb (http_parser *p)
{
  assert(p);
  if (messages[num_messages].should_keep_alive != http_should_keep_alive(p))
  {
    fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
                    "value in both on_message_complete and on_headers_complete "
                    "but it doesn't! ***\n\n");
    assert(0);
    abort();
  }

  if (messages[num_messages].body_size &&
      http_body_is_final(p) &&
      !messages[num_messages].body_is_final)
  {
    fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
                    "on last on_body callback call "
                    "but it doesn't! ***\n\n");
    assert(0);
    abort();
  }

  messages[num_messages].message_complete_cb_called = TRUE;

  messages[num_messages].message_complete_on_eof = currently_parsing_eof;

  num_messages++;
  return 0;
}

int
response_status_cb (http_parser *p, const char *buf, size_t len)
{
  assert(p);
  strlncat(messages[num_messages].response_status,
           sizeof(messages[num_messages].response_status),
           buf,
           len);
  return 0;
}

int
chunk_header_cb (http_parser *p)
{
  assert(p);
  int chunk_idx = messages[num_messages].num_chunks;
  messages[num_messages].num_chunks++;
  if (chunk_idx < MAX_CHUNKS) {
    messages[num_messages].chunk_lengths[chunk_idx] = p->content_length;
  }

  return 0;
}

int
chunk_complete_cb (http_parser *p)
{
  assert(p);

  /* Here we want to verify that each chunk_header_cb is matched by a
   * chunk_complete_cb, so not only should the total number of calls to
   * both callbacks be the same, but they also should be interleaved
   * properly */
  assert(messages[num_messages].num_chunks ==
         messages[num_messages].num_chunks_complete + 1);

  messages[num_messages].num_chunks_complete++;
  return 0;
}

static http_parser_settings settings =
  {.on_message_begin = message_begin_cb
  ,.on_header_field = header_field_cb
  ,.on_header_value = header_value_cb
  ,.on_url = request_url_cb
  ,.on_status = response_status_cb
  ,.on_body = body_cb
  ,.on_headers_complete = headers_complete_cb
  ,.on_message_complete = message_complete_cb
  ,.on_chunk_header = chunk_header_cb
  ,.on_chunk_complete = chunk_complete_cb
  };

void
parser_package_init()
{
  if((sem_init(&sem, 0, 1)) < 0) {
    perror("sem_init()");
    exit(1);
  }
}

int
parse_messages (enum http_parser_type type, int message_count, struct message *input_messages)
{
  http_parser *parser;

  sem_wait(&sem);

  num_messages = 0;

  messages = input_messages;

  switch(type)
  {
    case HTTP_REQUEST:
      parser = &request_parser;
      http_parser_init(parser, HTTP_REQUEST);
      break;
    case HTTP_RESPONSE:
      parser = &response_parser;
      http_parser_init(parser, HTTP_RESPONSE);
      break;
    default:
      exit(1);
  }

  // Concat the input messages
  size_t length = 0;
  int i;
  for (i = 0; i < message_count; i++) {
    length += strlen(messages[i].raw);
  }
  char *total = calloc(1, length + 1);

  for (i = 0; i < message_count; i++) {
    strcat(total, messages[i].raw);
  }

  // Parse the stream

  size_t traversed = 0;

  traversed = http_parser_execute(parser, &settings, total, length);

//  assert(HTTP_PARSER_ERRNO(parser) == HPE_OK);
//  assert(num_messages == message_count);

  free(total);

  sem_post(&sem);

  if(HTTP_PARSER_ERRNO(parser) != HPE_OK) return 2;
  if(num_messages != message_count) return 1;
  return 0;
}
