#include <iostream>
#include <string>
using namespace std;

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
      if(s.substr(5)=="type" || s.substr(5)=="echo" ||s.substr(5)=="exit"){
        cout<< s.substr(5)<<" is a shell builtin"<<endl;
      }
      else cout<< s.substr(5)<<": not found"<<endl;
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
  
