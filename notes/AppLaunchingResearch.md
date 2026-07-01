**Research: App Launching in C++**

**Overview**  
This document contains the proof-of-concept for launching external applications using C++ and the Windows API. This directly fulfills our MVP requirement to allow the launcher to open external programs/scripts. 

**How it Works**  
To launch apps, we are using the ShellExecuteA function from the Windows API (\<windows.h\>) 

*  It takes the name of the executable (e.g., \`"calc.exe"\` or \`"notepad.exe"\`).  
* The "open" parameter tells Windows to execute it.  
* SW\_SHOWNORMAL tells Windows to open the app in its default, visible state.  
* If the function returns a value greater than 32, it means the launch was successful.

**Prototype Code**  
\#include \<windows.h\>  
\#include \<iostream\>  
// this is the reusable test function  
bool launchAppTest(const char\* appName) {  
    std::cout \<\< "Testing launch for: " \<\< appName \<\< "..." \<\< std::endl;  
      
    // attempt to open the application  
    HINSTANCE result \= ShellExecuteA(NULL, "open", appName, NULL, NULL, SW\_SHOWNORMAL);  
    // according to windows API, if the return value is greater than 32, it was successful  
    if (reinterpret\_cast\<intptr\_t\>(result) \> 32\) {  
        std::cout \<\< "SUCCESS: " \<\< appName \<\< " is running\!\\n" \<\< std::endl;  
        return true;  
    } else {  
        std::cout \<\< "FAILED: Could not launch " \<\< appName \<\< ".\\n" \<\< std::endl;  
        return false;  
    }  
}  
int main() {  
    // run the test for calculator  
    launchAppTest("calc.exe");  
    // run the test for notepad  
    launchAppTest("notepad.exe");  
    return 0;  
}  
