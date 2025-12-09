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
    cout<<s<<": command not found"<<endl;
    continue;
  }
}
  
