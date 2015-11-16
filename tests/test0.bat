@echo off

set file=file1.c

:: Run clang
clang -pedantic -Wall -O0 "%file%" >out.txt 2>&1
if %errorlevel% neq 0 exit /B 1

findstr /c:"incompatible redeclaration" out.txt > NUL 2>&1 || ^
findstr /c:"ordered comparison between pointer" out.txt > NUL 2>&1 || ^
findstr /c:"eliding middle term" out.txt > NUL 2>&1 || ^
findstr /c:"end of non-void function" out.txt > NUL 2>&1 || ^
findstr /c:"invalid in C99" out.txt > NUL 2>&1 || ^
findstr /c:"specifies type" out.txt > NUL 2>&1 || ^
findstr /c:"should return a value" out.txt > NUL 2>&1 || ^
findstr /c:"too few argument" out.txt > NUL 2>&1 || ^
findstr /c:"too many argument" out.txt > NUL 2>&1 || ^
findstr /c:"return type of 'main" out.txt > NUL 2>&1 || ^
findstr /c:"uninitialized" out.txt > NUL 2>&1 || ^
findstr /c:"incompatible pointer to" out.txt > NUL 2>&1 || ^
findstr /c:"incompatible integer to" out.txt > NUL 2>&1 || ^
findstr /c:"type specifier missing" out.txt > NUL 2>&1
if %errorlevel% equ 0 exit /B 1

gcc -c -Wall -Wextra -O "%file%" >outa.txt 2>&1
if %errorlevel% neq 0 exit /B 1

findstr /c:"uninitialized" outa.txt > NUL 2>&1 || ^
findstr /c:"control reaches end" outa.txt > NUL 2>&1 || ^
findstr /c:"no semicolon at end" outa.txt > NUL 2>&1 || ^
findstr /c:"incompatible pointer" outa.txt > NUL 2>&1 || ^
findstr /c:"cast from pointer to integer" outa.txt > NUL 2>&1 || ^
findstr /c:"ordered comparison of pointer with integer" outa.txt > NUL 2>&1 || ^
findstr /c:"declaration does not declare anything" outa.txt > NUL 2>&1 || ^
findstr /c:"expects type" outa.txt > NUL 2>&1 || ^
findstr /c:"assumed to have one element" outa.txt > NUL 2>&1 || ^
findstr /c:"division by zero" outa.txt > NUL 2>&1 || ^
findstr /c:"pointer from integer" outa.txt > NUL 2>&1 || ^
findstr /c:"incompatible implicit" outa.txt > NUL 2>&1 || ^
findstr /c:"excess elements in struct initializer" outa.txt > NUL 2>&1 || ^
findstr /c:"comparison between pointer and integer" outa.txt > NUL 2>&1
if %errorlevel% equ 0 exit /B 1

findstr /c:"goto" "%file%" > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

:: Variant is interesting
exit /B 0
