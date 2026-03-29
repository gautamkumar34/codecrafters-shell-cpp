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
#include <termios.h>  // For raw mode
#include <set>        // To keep completions unique and sorted
#include <algorithm>  // For sorting

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

string longest_common_prefix(const vector<string>& strs){
  if(strs.empty())return "";
  string prefix = strs[0];
  for(size_t i = 0;i<strs.size();i++){
    while(strs[i].find(prefix)!=0){
      prefix = prefix.substr(0,prefix.length()-1);
      if(prefix.empty())return "";
    }
  }
  return prefix;
}

vector<string> command_completion(const string& prefix){
  set<string>matches;
  vector<string>buitins = {"cd", "echo","exit","pwd","type"};
  for(auto& b: buitins){
    if(b.find(prefix)==0)matches.insert(b);
  }

  const char* path_env = getenv("PATH");
  if(path_env){
    stringstream ss(path_env);
    string dir;
    while(getline(ss,dir,':')){
      if(dir.empty())continue;
      DIR* dp = opendir(dir.c_str());
      if(dp){
        struct dirent* ep;
        while((ep=readdir(dp)) != nullptr){
          string name = ep->d_name;
          if(name == "." || name == "..")continue;
          if(name.find(prefix)==0){
            matches.insert(name);
          }
        }
        closedir(dp);
      }
    }
  }
  return vector<string>(matches.begin(),matches.end());
}

vector<string> file_completion(const string& prefix ){
  set<string>matches;
  string dir_path= ".";
  string file_prefix = prefix;

  size_t last_slash = prefix.find_last_of('/');
  if(last_slash != string::npos){
    dir_path= prefix.substr(0, last_slash+1);
    file_prefix = prefix.substr(last_slash+1);
    if(dir_path.empty())dir_path = "/";
  }

  DIR* dp = opendir(dir_path.c_str());
  if(dp){
    struct dirent* ep;
    while((ep= readdir(dp)) != nullptr){
      string name = ep->d_name;
      if(name == "." || name == "..")continue;
      if(name.find(file_prefix)==0){
        string path_to_stat = (dir_path== "." ? name : dir_path +name);
        struct stat statbuff;
        if(stat(path_to_stat.c_str(), &statbuff)==0){
          if(S_ISDIR(statbuff.st_mode)){
            name += "/";
          }
        }
        matches.insert(name);
      }
    }
    closedir(dp);
  }
  vector<string> results;
  for(auto& m : matches){
    if(last_slash != string::npos)results.push_back(dir_path+m);
    else results.push_back(m);
  }
  return results;
}

struct termios orig_terminos;

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_terminos);
}

void enableRawMode(){
  tcgetattr(STDIN_FILENO, &orig_terminos);
  atexit(disableRawMode);
  struct termios raw = orig_terminos;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

string read_line_raw(){
  string input;
  char c;
  bool last_was_tab = false;

  while(read(STDIN_FILENO, &c , 1)==1){
    if(c=='\r')continue;
    if(c == '\n'){
      cout<<"\n";
      break;
    }
    else if(c==4){
      if(input.empty())exit(0);
    }
    else if(c == 127 || c == '\b'){
      if(!input.empty()){
        input.pop_back();
        cout<<"\b \b";
      }
      last_was_tab = false;
    }
    else if(c == '\t'){
      size_t last_space = input.find_last_of(' ');
      bool is_cmd = (last_space == string::npos);
      string prefix = is_cmd ? input : input.substr(last_space +1);

      vector<string> matches = is_cmd ? command_completion(prefix) : file_completion(prefix);
      if(matches.empty()){
        cout<<"\a";
      }
      else if(matches.size()==1){
        string match = matches[0];
        string to_add = match.substr(prefix.length()) + " ";
        if(!match.empty() && match.back() != '/'){
          to_add += " ";
        }
        input += to_add;
        cout<<to_add;
        last_was_tab = false;
      }
      else{
        string lcp = longest_common_prefix(matches);
        if(lcp.length()> prefix.length()){
          string to_add = lcp.substr(prefix.length());
          input += to_add;
          cout<<to_add;
          last_was_tab = false;
        }
        else{
          if(last_was_tab){
            cout<<"\n";
            for(size_t i=0;i<matches.size();i++){
              cout<<matches[i]<<" ";
            }
            cout<<"\n$ "<<input;
            last_was_tab = false;
          }
          else{
            cout<<"\a";
            last_was_tab = true;
          }
        }
      }
      if(c =='\t')continue;
    }
    else{
      input += c;
      cout<<c;
      last_was_tab = false;
    }
  }
  return input;
}

void execute_command(vector<string> raw_args, bool is_forked){
  if(raw_args.empty())return;
    vector<string>args;

    // extract the Redirections < , > , >>, 2>.
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
    if(args.empty()){
      if(is_forked)_exit(0);
      return;
    }
    string cmd = args[0];

    // Apply Redirection

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
     // Built-ins and external command executions

    if(cmd == "exit"){
      int code = (args.size()>1)? stoi(args[1]) :0;
      exit(code);
    }
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
        if(is_forked){
          vector<char*>c_args;
          for(auto& a: args){
            c_args.push_back(&a[0]);
          }
          c_args.push_back(nullptr);
          execv(full_path.c_str(),c_args.data());
          perror("execv");
          _exit(1);
        }
        else{
          pid_t pid = fork();
          if(pid ==0){
            vector<char*>c_args;
            for(auto& a :args){
              c_args.push_back(&a[0]);
            }
            c_args.push_back(nullptr);
            execv(full_path.c_str(), c_args.data());
            perror("execv");
            _exit(1);
          }
          else {
            waitpid(pid , nullptr,0);
          }
        }
      }
      else{
        cout<<cmd<<": command not found"<<endl;
      }
    }

    //Restore Descriptors

    dup2(saved_out, STDOUT_FILENO);
    dup2(saved_err, STDERR_FILENO);
    dup2(saved_in, STDIN_FILENO);
    close(saved_out); close(saved_err); close(saved_in);

    if(is_forked)_exit(0);
}

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;
  
  enableRawMode();

  while(true){
    cout << "$ ";
    string s = read_line_raw();
    if(!s.empty() && s.back()=='\r')s.pop_back();
    
    vector<string> raw_args = parse_s(s);
    if(raw_args.empty())continue;

    vector<vector<string>>commands;
    vector<string>curr_cmd;

    for(const string& token : raw_args ){
      if(token == "|"){
        commands.push_back(curr_cmd);
        curr_cmd.clear();
      }
      else{
        curr_cmd.push_back(token);
      }
    }
    commands.push_back(curr_cmd);

    if(commands.size()==1){// no piping
      execute_command(commands[0],false);
    }
    else{
      int prev_read_fd = -1;
      vector<pid_t>children;
      for(size_t i = 0; i<commands.size();i++){
        int pipe_fd[2];
        if(i<commands.size()-1){
          pipe(pipe_fd);
        }
        cout.flush();
        pid_t pid = fork();
        if(pid==0){
          if(prev_read_fd != -1){
            dup2(prev_read_fd,STDIN_FILENO);
            close(prev_read_fd);
          }
          if(i < commands.size()-1){
            dup2(pipe_fd[1],STDOUT_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);
          }
          execute_command(commands[i],true);
          exit(0);
        }
        else{
          children.push_back(pid);
          if(prev_read_fd != -1)close(prev_read_fd);
          if(i < commands.size()-1){
            close(pipe_fd[1]);
            prev_read_fd= pipe_fd[0];
          }
        }
      }
      for(pid_t child_pid : children){
        waitpid(child_pid,nullptr,0);
      }
    }
    
  }
  return 0;


}
  
