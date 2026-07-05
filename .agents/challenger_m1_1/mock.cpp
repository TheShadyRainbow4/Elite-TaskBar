#include <windows.h>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    // Write a log file to indicate execution
    std::ofstream f("C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\mock_run.txt");
    f << "StartMenu.exe mock executed successfully!" << std::endl;
    for (int i = 0; i < argc; ++i) {
        f << "Arg[" << i << "]: " << argv[i] << std::endl;
    }
    f.close();
    return 0;
}
