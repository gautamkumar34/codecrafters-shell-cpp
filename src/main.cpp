#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdlib>   // for getenv
#include <unistd.h>  // for access()

using namespace std;

string get_path(string task) {
    const char* path_env = getenv("PATH");
    if(path_env==nullptr){
      cout<<task<<": not found"<<endl;
      return;
    }

    //split by delimiter
    string path_str(path_env);
    stringstream ss(path_str);
    string directory ;

    while (getline(ss, directory, ':')) {
        if(directory.empty())continue;

        string full_path_str = directory + "/" + task;
        struct stat file_stat;
        if(stat(full_path_str.c_str(), &file_stat)==0){
          if(S_ISREG(file_stat.st_mode)){
            if(access(full_path_str.c_str(),X_OK)==0){
              return full_path_str;
            }
          }
          
        }
    }

    return "";
}

vector<string>parse_s(string s){
  vector<string>args;
  stringstream ss(s);
  string parts;
  while(ss >>parts){
    args.push_back(parts);
  }
  return args;
}

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  while(true){
    cout << "$ ";
    string s;
    getline(cin,s);
    
    vector<string> args = parse_s(s);
    string cmd = args[0];

    if(cmd == "exit")break;
    if(cmd=="type"){
      string task = args[1];                                     
      if(task=="type" || task=="echo" ||task=="exit"){                 
        cout<< cmd<<" is a shell builtin"<<endl;                    
      }                                                             
      else {                                                        
        get_path(task);                                              
      }               
    }
    else if(cmd=="echo"){
      cout<<s.substr(5)<<endl;
    }
    else{
      string full_path = get_path(cmd) ;
      if(!full_path.empty()){
        pid_t pid = fork();
        if(pid ==0){
          vector<char*>c_args;
          for(auto& a :args){
            c_args.push_back(&a[0]);
          }
          c_args.push_back(nullptr);
          execv(full_path.c_str(), c_args.data());
          perror("execv");
          exit(1);
        }
        else {
          waitpid(pid , nullptr,0);
        }
      }
      else{
        cout<<cmd<<":not found"<<endl;
      }
    }
    continue;
  }
  return 0;
}
  
