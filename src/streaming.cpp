#include "streaming.h"

#include "curl.h"

#include <string>
#include <iostream>
#include <string.h>
using std::string;
using std::cout;
using std::endl;

const char* buffer_queue::front() const {
  if(data_queue_.empty())
    return NULL;
  else
    return data_queue_.front();
}
size_t buffer_queue::front_size() const {
  if(size_queue_.empty())
    return 0;
  else
    return size_queue_.front();
}
size_t buffer_queue::front_offset() const {
  return front_offset_;
}
void buffer_queue::set_front_offset(size_t s){
  front_offset_ += s;
}
void buffer_queue::pop() {
  if(not data_queue_.empty()){
    char* buffer = data_queue_.front();
    data_queue_.pop();
    size_queue_.pop();
    front_offset_ = 0;
    empty_buffers_.push(buffer);
  }
}
bool buffer_queue::eof() const {
  return is_eof_;
}
void buffer_queue::set_eof(){
  is_eof_ = true;
}
void buffer_queue::push_buffer(char* ptr, size_t s){
  char* buffer;
  if(not empty_buffers_.empty()){
    buffer = empty_buffers_.top();
    empty_buffers_.pop();
  }else{
    buffer = (char*)malloc(sizeof(char)*default_buffer_size_);
  }
  if(s <= default_buffer_size_){
    memcpy(buffer, ptr, s);
    data_queue_.push(buffer);
    size_queue_.push(s);
  }else{
    memcpy(buffer, ptr, default_buffer_size_);
    data_queue_.push(buffer);
    size_queue_.push(default_buffer_size_);
    push_buffer(ptr+default_buffer_size_, s-default_buffer_size_);
  }
}

buffer_queue::buffer_queue(size_t bufsize)
  :default_buffer_size_(bufsize),
   front_offset_(0),
   is_eof_(false)
{
}
buffer_queue::~buffer_queue(){
  while(not data_queue_.empty()){
    free(data_queue_.front());
    data_queue_.pop();
  }
  while(not empty_buffers_.empty()){
    free(empty_buffers_.top());
    empty_buffers_.pop();
  }
}


void * streaming_handler(void* ptr){
  int result;
  streaming_handler_data * userdata = (streaming_handler_data*)ptr;

  cout << "start streaming handler" << endl;

  pthread_mutex_lock(&(userdata->m));
  //  curl_easy_setopt(curl, CURLOPT_FILE, f);
  curl_easy_setopt(userdata->curl, CURLOPT_WRITEFUNCTION, got_data_chunk);
  curl_easy_setopt(userdata->curl, CURLOPT_WRITEDATA, (void*)current_buffer);

  struct curl_slist * slist = NULL;
  for(vector<std::string>::const_iterator it=userdata->request_headers.begin();
      it != userdata->request_headers.end(); ++it){
    slist = curl_slist_append(slist, it->c_str());
  }
  curl_easy_setopt(userdata->curl, CURLOPT_HTTPHEADER, slist);
  curl_easy_setopt(userdata->curl, CURLOPT_URL, userdata->url.c_str());
  pthread_mutex_unlock(&(userdata->m));

  result = my_curl_easy_perform(userdata->curl, NULL, NULL);

  pthread_mutex_lock(&(userdata->m));
  current_buffer->set_eof();
  pthread_mutex_unlock(&(userdata->m));
  cout << "end streaming handler" << endl;
  //  if(result != 0) {
  destroy_curl_handle(userdata->curl);
  return NULL;
    //return -result;
}

size_t got_data_chunk(char* ptr, size_t size, size_t nmenb, void* userdata){
  buffer_queue * q = (buffer_queue*)userdata;
  cout << ">" << endl;
  pthread_mutex_lock(current_mutex);
  q->push_buffer(ptr, size*nmenb);
  pthread_mutex_unlock(current_mutex);
  cout << "got data chunk: " << size*nmenb << " bytes got, " << current_buffer << " " << q << endl;
  return size*nmenb;
}

int start_streaming_read(const std::string& url,
			 const vector<std::string> & request_headers){
  //			 const struct curl_slist* slist){
  cout << "start streaming_read" << endl;
  streaming_handler_data * userdata = new streaming_handler_data;
  userdata->curl = create_curl_handle();
  userdata->q = current_buffer;
  userdata->request_headers = request_headers; //copy
  userdata->url = url;

  current_mutex = &(userdata->m);
  pthread_mutex_init(current_mutex, NULL);
  pthread_create(&(userdata->pid), NULL, streaming_handler, (void*)userdata);
  return 0;
}

pthread_mutex_t * current_mutex;
buffer_queue * current_buffer = new buffer_queue(1024*1024);
