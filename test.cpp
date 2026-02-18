#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main()
{
    vector<string> msg {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};
    for (const string& word : msg)
    {
        cout << word << " ";
    }

    for (int i = 0; i < 1000000000; i++)
    {
        cout << i << endl;
    }

    cout << endl;
}
