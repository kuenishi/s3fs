#pragma once

#include <pthread.h>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include <map>

#include <curl/curl.h>

using std::queue;
using std::stack;
using std::vector;

//
// streaming_handler => queue => read(2)
//

enum stream_types {
  DATA,
  EOS
};


class buffer_queue {
 public:
  const char* front() const;
  size_t front_size() const;
  size_t front_offset() const;
  void set_front_offset(size_t s);
  void pop();
  bool eof() const ;
  void set_eof();
  void push_buffer(char* ptr, size_t s);
  buffer_queue(size_t bufsize);
  ~buffer_queue();
 private:
  size_t default_buffer_size_;
  size_t front_offset_;
  bool is_eof_;
  queue<char*> data_queue_;
  queue<size_t> size_queue_;
  stack<char*> empty_buffers_;
};


struct streaming_handler_data {
  pthread_t pid;
  buffer_queue * q;
  CURL * curl;
  std::string url;
  vector<std::string> request_headers;
  pthread_mutex_t m;
};


static void* streaming_handler(void*);
static size_t got_data_chunk(char* ptr, size_t size, size_t nmemb, void* userdata);

int start_streaming_read(const std::string&,
			 const vector<std::string>&);

extern pthread_mutex_t * current_mutex;
extern buffer_queue * current_buffer;
