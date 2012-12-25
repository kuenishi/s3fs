#pragma once

#include <curl/curl.h>

#include "streaming.h"

#include <string>
using std::string;

namespace s3 {

  class file {
  public:
    file(const string& service_path,
	 const string& bucket, const string& path);
    ~file();
    void set_method(const char*);
    void do_request();

    // void pread();
    int open(unsigned int flags);

  private:
    string service_path_;
    string bucket_;
    string path_;
    string method_;
    unsigned int flags_;
    CURL * curl_;

    buffer_queue q_;
  };

  class large_file {
  };
  
}
