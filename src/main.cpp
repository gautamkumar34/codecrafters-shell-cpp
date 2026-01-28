#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <experimental/filesystem>
#include <cstdlib>   // for getenv
#include <unistd.h>  // for access()

using namespace std;
namespace fs = std::experimental::filesystem;

void get_path(string cmd) {
    const char* path_env = getenv("PATH");
    if(path_env==nullptr){
      cout<<cmd<<": not found"<<endl;
      return;
    }

    //split by delimiter
    string path_str(path_env);
    stringstream ss(path_str);
    string directory ;

    while (getline(ss, directory, ':')) {
        if(directory.empty())continue;

        fs::path full_path = fs::path(directory) /cmd;
        if(fs::exists(full_path) && !fs::is_directory(full_path)){
          if(access(full_path.c_str(),X_OK)==0){
            cout<<cmd<<" is "<<full_path.string()<<endl;
            return;
          }
        }
    }

    cout<<cmd<<": not found"<<endl;
}

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  while(true){
    cout << "$ ";
    string s;
    getline(cin,s);
    
    if(s == "exit")break;
    if(s.substr(0,4)=="type"){
      string cmd = s.substr(5);                                     
      if(cmd=="type" || cmd=="echo" ||cmd=="exit"){                 
        cout<< cmd<<" is a shell builtin"<<endl;                    
      }                                                             
      else {                                                        
        get_path(cmd);                                              
      }               
    }
    else if(s.substr(0,4)=="echo"){
      cout<<s.substr(5)<<endl;
    }
    else{
      cout<<s<<": command not found"<<endl;
    }
    continue;
  }
}
  
