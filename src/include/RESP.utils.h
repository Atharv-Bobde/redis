//#pragma once is a non-standard but widely supported preprocessor directive that tells the compiler to include a header file only once per compilation unit, even if it’s included multiple times.
#pragma once
#include <string>
#include <vector>

using namespace std;

void parseRESPArray(const string& input,vector<string> &result);
void parseBulkStrings(const string& input,vector<string> &result) ;
string toRESPBulkStrings(const vector<string>& items,int start,int end);
string toRESPArray(vector<string> arr);