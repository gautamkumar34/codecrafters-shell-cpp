#include <iostream>
#include <string>
using namespace std;

int main() {
  // Flush after every std::cout / std:cerr
  cout << std::unitbuf;
  cerr << std::unitbuf;

  while(true){
    cout << "$ ";
    string s,s1,s2;
    getline(cin,s);
    for(int i =0;i<s.length();i++){
      if(s[i]==' '){
        s1 = s.substr(0,i);
        s2 = s.substr(i+1,s.length()-1);
        break;
      }
    }
    if(s == "exit")break;
    else{
      if(s1=="echo"){
        cout<<s2<<endl;
      }
      else{
        cout<<s<<": command not found"<<endl;
      }
    }
    continue;
  }
}
  
