\#include \<windows.h\>  
\#include \<string\>  
\#include \<iostream\>

class AppAction {  
private:  
    std::string applicationPath;

public:  
    AppAction(const std::string& path) : applicationPath(path) {}

    void execute() const {  
        std::cout \<\< "\[Action\] Launching: " \<\< applicationPath \<\< "\\n";  
        ShellExecuteA(NULL, "open", applicationPath.c\_str(), NULL, NULL, SW\_SHOWNORMAL);  
    }  
};

void onButtonClickHandler(const AppAction& actionToRun) {  
    std::cout \<\< "\[UI\] Button was clicked by the user.\\n";  
    actionToRun.execute();  
}

int main() {  
    AppAction openNotepad("notepad.exe");  
    AppAction openCalculator("calc.exe");

    std::cout \<\< "--- Application Running \---\\n\\n";

    onButtonClickHandler(openNotepad);

    std::cout \<\< "\\n--- Simulating another button click \---\\n\\n";

    onButtonClickHandler(openCalculator);

    return 0;  
}

