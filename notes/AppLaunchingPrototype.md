{\rtf1\ansi\ansicpg1252\cocoartf2870
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\froman\fcharset0 TimesNewRomanPS-BoldMT;\f1\froman\fcharset0 Times-Roman;\f2\froman\fcharset0 TimesNewRomanPSMT;
}
{\colortbl;\red255\green255\blue255;\red0\green0\blue0;}
{\*\expandedcolortbl;;\cssrgb\c0\c0\c0;}
{\*\listtable{\list\listtemplateid1\listhybrid{\listlevel\levelnfc23\levelnfcn23\leveljc0\leveljcn0\levelfollow0\levelstartat0\levelspace360\levelindent0{\*\levelmarker \{disc\}}{\leveltext\leveltemplateid1\'01\uc0\u8226 ;}{\levelnumbers;}\fi-360\li720\lin720 }{\listname ;}\listid1}}
{\*\listoverridetable{\listoverride\listid1\listoverridecount0\ls1}}
\margl1440\margr1440\vieww19540\viewh13000\viewkind0
\deftab720
\pard\pardeftab720\partightenfactor0

\f0\b\fs29\fsmilli14667 \cf0 \expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 Research: App Launching in C++
\f1\b0\fs24 \
\

\f0\b\fs29\fsmilli14667 Overview
\f1\b0\fs24 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 This document contains the proof-of-concept for launching external applications using C++ and the Windows API. This directly fulfills our MVP requirement to allow the launcher to open external programs/scripts.\'a0
\f1\fs24 \
\

\f2\fs29\fsmilli14667 ## How it Works
\f1\fs24 \

\f2\fs29\fsmilli14667 To launch apps, we are using the ShellExecuteA function from the Windows API (<windows.h>)\'a0
\f1\fs24 \
\pard\tx220\tx720\pardeftab720\li720\fi-720\partightenfactor0
\ls1\ilvl0
\f2\fs29\fsmilli14667 \cf0 \kerning1\expnd0\expndtw0 \outl0\strokewidth0 {\listtext	\uc0\u8226 	}\expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 \'a0It takes the name of the executable (e.g., `"calc.exe"` or `"notepad.exe"`).\
\ls1\ilvl0\kerning1\expnd0\expndtw0 \outl0\strokewidth0 {\listtext	\uc0\u8226 	}\expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 The "open" parameter tells Windows to execute it.\
\ls1\ilvl0\kerning1\expnd0\expndtw0 \outl0\strokewidth0 {\listtext	\uc0\u8226 	}\expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 SW_SHOWNORMAL tells Windows to open the app in its default, visible state.\
\ls1\ilvl0\kerning1\expnd0\expndtw0 \outl0\strokewidth0 {\listtext	\uc0\u8226 	}\expnd0\expndtw0\kerning0
\outl0\strokewidth0 \strokec2 If the function returns a value greater than 32, it means the launch was successful.\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 ## Prototype Code
\f1\fs24 \

\f2\fs29\fsmilli14667 Here is a standalone test script that successfully opens both Calculator and Notepad.\'a0
\f1\fs24 \
\

\f2\fs29\fsmilli14667 #include <windows.h>
\f1\fs24 \

\f2\fs29\fsmilli14667 #include <iostream>\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 // this is the reusable test function
\f1\fs24 \

\f2\fs29\fsmilli14667 bool launchAppTest(const char* appName) \{
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0std::cout << "Testing launch for: " << appName << "..." << std::endl;
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0// attempt to open the application
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0HINSTANCE result = ShellExecuteA(NULL, "open", appName, NULL, NULL, SW_SHOWNORMAL);\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 \'a0\'a0\'a0\'a0// according to windows API, if the return value is greater than 32, it was successful
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0if (reinterpret_cast<intptr_t>(result) > 32) \{
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\'a0\'a0\'a0\'a0std::cout << "SUCCESS: " << appName << " is running!\\n" << std::endl;
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\'a0\'a0\'a0\'a0return true;
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\} else \{
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\'a0\'a0\'a0\'a0std::cout << "FAILED: Could not launch " << appName << ".\\n" << std::endl;
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\'a0\'a0\'a0\'a0return false;
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0\}
\f1\fs24 \

\f2\fs29\fsmilli14667 \}\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 int main() \{
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0// run the test for calculator
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0launchAppTest("calc.exe");\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 \'a0\'a0\'a0\'a0// run the test for notepad
\f1\fs24 \

\f2\fs29\fsmilli14667 \'a0\'a0\'a0\'a0launchAppTest("notepad.exe");\
\pard\pardeftab720\partightenfactor0

\f1\fs24 \cf0 \
\pard\pardeftab720\partightenfactor0

\f2\fs29\fsmilli14667 \cf0 \'a0\'a0\'a0\'a0return 0;
\f1\fs24 \

\f2\fs29\fsmilli14667 \}
\f1\fs24 \
\
}