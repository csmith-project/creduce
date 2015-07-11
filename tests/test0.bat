@echo off

:: Run clang
clang -pedantic -Wall -O0 "%1" >out.txt 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/incompatible redeclaration/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/ordered comparison between pointer/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/eliding middle term/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/end of non-void function/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/invalid in C99/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/specifies type/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/should return a value/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/too few argument/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/too many argument/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/return type of 'main/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/uninitialized/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/incompatible pointer to/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/incompatible integer to/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/type specifier missing/) {$found=1; exit}" out.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

gcc -c -Wall -Wextra -O "%1" >outa.txt 2>&1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/uninitialized/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/control reaches end/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/no semicolon at end/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/incompatible pointer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/cast from pointer to integer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/ordered comparison of pointer with integer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/declaration does not declare anything/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/expects type/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/assumed to have one element/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/division by zero/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/pointer from integer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/incompatible implicit/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/excess elements in struct initializer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 1} else {exit 0}} if(/comparison between pointer and integer/) {$found=1; exit}" outa.txt > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

perl -ne "END {if($found) {exit 0} else {exit 1}} if(/goto/) {$found=1; exit}" "%1" > NUL 2>&1
if %errorlevel% neq 0 exit /B 1

:: Variant is interesting
exit /B 0
