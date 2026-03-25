#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <cstdlib>   // for getenv
#include <unistd.h>   // for access()
#include <limits.h>  
#include <fcntl.h>

using namespace std;

string get_path(string task) {
    const char* path_env = getenv("PATH");
    if(path_env==nullptr){
      return "";
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
   vector<string> args;
    string curr;
    bool in_sq = false; 
    bool in_dq = false; 
    bool in_token = false; 

    for(size_t i=0;i<s.length();++i){
        char c = s[i];
        if(c =='\'' && !in_dq){
            in_sq = !in_sq;    
            in_token = true;   
        }
        else if(c == '"' && !in_sq){
            in_dq = !in_dq;    
            in_token = true;
        }
        else if(isspace(c) && !in_sq && !in_dq){
          if(in_token){
            args.push_back(curr);
            curr.clear();
            in_token = false;
          }
        }
        else{
          curr += c;
          in_token = true;
        }
    }
    
    if(in_token){
      args.push_back(curr);
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
    if(!getline(cin,s))break;
    
    vector<string> raw_args = parse_s(s);

    if(raw_args.empty())continue;
    vector<string>args;
    string file_in , file_out, file_err;
    bool append_out = false, append_err = false;

    for(size_t i = 0 ; i<raw_args.size();i++){
      if(raw_args[i]=="<" && i+1 < raw_args.size()){
        file_in = raw_args[++i];
      }
      else if((raw_args[i]==">" || raw_args[i]=="1>") && i+1 < raw_args.size()){
        file_out = raw_args[++i];
        append_out= false;
      }
      else if((raw_args[i]==">>" || raw_args[i]=="1>>") && i+1 < raw_args.size()){
        file_out = raw_args[++i];
        append_out= true;
      }
      else if(raw_args[i]=="2>" && i+1 < raw_args.size()){
        file_err= raw_args[++i];
        append_err = false;
      }
      else if(raw_args[i]=="2>>" && i+1 < raw_args.size()){
        file_err= raw_args[++i];
        append_err = true;
      }
      else{
        args.push_back(raw_args[i]);
      }
    }
    if(args.empty())continue;
    string cmd = args[0];

    int saved_out= dup(STDOUT_FILENO);
    int saved_err = dup(STDERR_FILENO);
    int saved_in = dup(STDIN_FILENO);

    if(!file_out.empty()){
      int flags = O_WRONLY | O_CREAT | (append_out ? O_APPEND : O_TRUNC);
      int fd = open(file_out.c_str(), flags, 0644);
      if(fd != -1){dup2(fd,STDOUT_FILENO); close(fd);}
      else perror("open");
    }
    if(!file_err.empty()){
      int flags = O_WRONLY | O_CREAT | (append_err ? O_APPEND : O_TRUNC);
      int fd = open(file_err.c_str(), flags, 0644);
      if(fd != -1){dup2(fd,STDERR_FILENO); close(fd);}
      else perror("open");
    }
    if(!file_in.empty()){
      int fd = open(file_in.c_str(), O_RDONLY);
      if(fd != -1){dup2(fd,STDIN_FILENO); close(fd);}
      else perror("open");
    }

    if(cmd == "exit")break;
    if(cmd=="type"){
      string task = args[1];                                     
      if(task=="type" || task=="echo" ||task=="exit" || task =="pwd" || task =="cd"){                 
        cout<<task<<" is a shell builtin"<<endl;                    
      }                                                             
      else {          
        string full_path = get_path(task);
        if(!full_path.empty()){
          cout<<task<<" is "<<full_path<<endl;
        }
        else cout<<task<<": not found"<<endl;                                                                                         
      }               
    }
    else if(cmd=="echo"){
      for(size_t i = 1; i < args.size(); ++i){
          cout << args[i];
          if (i < args.size() - 1) cout << " ";
      }
      cout << "\n";
          
    }
    else if(cmd == "pwd"){
      char cwd[PATH_MAX];
      if(getcwd(cwd , sizeof(cwd)) != nullptr){
        cout<<cwd<<endl;
      }
      else perror("pwd");
    }
    else if(cmd == "cd"){
      if(args.size()<2){
        const char* home = getenv("HOME");
        if(home)chdir(home);
      }
      else{
        string path = args[1];
        if(path == "~"){
          const char* home = getenv("HOME");
          if(home){
            path = string(home);
          }
        }

        if(chdir(path.c_str())!= 0){
          cout<<"cd: "<<path<<": No such file or directory"<<endl;
        }
      }
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
        cout<<cmd<<": command not found"<<endl;
      }
    }
    dup2(saved_out, STDOUT_FILENO);
    dup2(saved_err, STDERR_FILENO);
    dup2(saved_in, STDIN_FILENO);
    close(saved_out); close(saved_err); close(saved_in);
  }
  return 0;


}
  
