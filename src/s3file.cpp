#include "s3file.h"
#include "streaming.h"

#include "curl.h"
#include <vector>
#include "string_util.h"

namespace s3{

  file::file(const string& service_path,
	     const string& bucket, const string& path)
    :
    service_path_(service_path),
    bucket_(bucket),
    path_(path),
    method_("GET"),
    curl_(NULL),
    q_(65536)
  {
  }

  file::~file(){
    destroy_curl_handle(curl_);
  }

  void file::set_method(const char* method){
    method_ = method;
  }
  void file::do_request()
  {
  }

  // only for O_RDONLY case!
  int file::open(unsigned int flags){
    flags_ = flags;
    //    char * s3_realpath = get_realpath(path_);
    string resource = urlEncode(service_path_ + bucket_ + path_);
    string url = host + resource;

    std::vector<string> request_headers;

    string date = get_date();
    string my_url = prepare_url(url.c_str());
    request_headers.push_back("Date: " + date);
    request_headers.push_back("Content-Type: ");

    struct curl_slist * slist = NULL;
    for(vector<std::string>::const_iterator it=request_headers.begin();
	it != request_headers.end(); ++it){
      slist = curl_slist_append(slist, it->c_str());
    }
    
    if(public_bucket.substr(0,1) != "1") {
      string signature = "Authorization: AWS " + AWSAccessKeyId + ":" +
	calc_signature(method_, "", date, slist, resource);
      slist = curl_slist_append(slist, signature.c_str());
			
      request_headers.push_back(signature);
    }
    return start_streaming_read(my_url, request_headers);
  }

}
