# C++ Coding Standards

## Header Files:
-Every .ccp file should have an affiliated .hpp with the same name  
-Some exceptions such as a small .cpp file with just a main()  
-Order of Include Headers: Standard C++ Library headers first, project headers last  
## Indentation:
-Only use spaces for indentation, 2 spaces at a time.  
-Do not use tabs at all in your code  

## Function Declaration:
-Return type, function name, and parameters should all be on the same line.  
-Choose short concise parameter names that are easy to understand    
-Before each function declaration, add a brief comment that explains the purpose, input and output of your function.  
## Local Variables:
-Try to place each variable in the narrowest scope possible  
-Initialize variables in their declaration  
-Declare variables as close to their first use as possible to allow readers to easily find a variables use and associated declaration  
## Naming: 
-Use descriptive names for variables and functions.  
-Use PascalCase for class names.  
-Use camelCase for variables and functions.  
-Use UPPER_CASE for constants and macros.  
## Braces:
-Always use braces for if, else, for, while, and do-while statements, even for a single statement.  
## Line Length:
-Keep lines under 100 characters whenever practical.  
## Comments:
-Write comments that explain why the code exists, not what every line does.  
-Remove outdated comments when code changes.  
## Magic Numbers:
-Avoid hardcoded numeric values.  
-Use named constants or constexpr variables instead.  
## Classes:
-Keep data members private unless there is a strong reason not to.  
-Keep public interfaces concise and easy to understand.  
## Formatting:
-Leave one blank line between function definitions.  
-Do not leave trailing whitespace at the end of lines.  






