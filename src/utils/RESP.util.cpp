#include "RESP.utils.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cctype>

using namespace std;

void parseRESPArray(const string& input,vector<string> &result) {

    stringstream ss(input);
    char type;
    ss >> type;
  
    if (type != '*') {
       cerr <<"Invalid RESP array format: expected '*', got '" + string(1, type) + "'";
    }
  
   
    int array_size;
    ss >> array_size;
    if (ss.fail() || array_size < 0) {
        cerr <<"Invalid RESP array size";
    }
  
    // Consume the newline character after reading the array size *2\r\n$4\r\nECHO\r\n$3\r\nhey\r\n
    ss.ignore(numeric_limits<streamsize>::max(), '\n');
    char c =ss.peek();
  
    for (int64_t i = 0; i < array_size; ++i) {
        if (ss.peek() != '$') {
           cerr <<"Invalid RESP array element format: missing string indicator";
        }
        ss >> type; // Read '$'
         if (type != '$') {
           cerr <<"Invalid RESP array element format: expected '$', got '" + string(1, type) + "'";
        }
  
        int64_t string_length;
        ss >> string_length;
         if (ss.fail() || string_length < 0) {
           cerr <<"Invalid RESP string length";
        }
         // Consume the newline character after reading the string length
        ss.ignore(numeric_limits<streamsize>::max(), '\n');
  
        string element;
        element.resize(string_length);
        ss.read(&element[0], string_length);
  
  
        result.push_back(element);
  
        // Consume the newline character after reading the string data
         ss.ignore(numeric_limits<streamsize>::max(), '\n');
    }
  
  }
  
  string toRESPBulkStrings(const vector<string>& items,int start,int end) {
    string resp;
    for (int i = start; i < end; ++i) {
      const string& item = items[i];
        resp += "$" + to_string(item.size()) + "\r\n" + item + "\r\n";
    }
    return resp;
  }
  
  void parseBulkStrings(const string& input,vector<string> &result) {
    size_t pos = 0;
  
    while (pos < input.size()) {
        if (input[pos] == '$') {
            // Find end of length line
            size_t len_end = input.find("\r\n", pos);
            if (len_end == string::npos) break;
  
            // Parse length
            int length = stoi(input.substr(pos + 1, len_end - pos - 1));
            pos = len_end + 2;
  
            // Check bounds and extract the string
            if (pos + length > input.size()) break;
            string str = input.substr(pos, length);
            result.push_back(str);
  
            pos += length + 2; // Move past content and \r\n
        } else {
            break;
        }
    }
  }

  string toRESPArray(vector<string> arr, bool useBulkStrings) {
    int n= arr.size();
    string response="";
    if(useBulkStrings){
      response= toRESPBulkStrings(arr,0,n);
    }else{
      for(int i=0;i<n;i++){
        response+=arr[i];
      }
    }
    string respArrayString="*" + to_string(n) + "\r\n"+response;
    return respArrayString;
  }