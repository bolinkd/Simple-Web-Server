Assignment 1
Name: Dustin Bolink
Student Number: V00747883
Lab Section: B04

To install:
  make

To run:
  ./sws  <port> <directory>

  port: any port number can be used as the program will allow for reuse of a port
  directory: if it ends in '/' it is assumed to be a directory, if not implicit "index.html" is added after

  returns:
  400 - Bad Request: either .. is used or is not using GET or HTTP/1.0
  404 - File Not Found: good request but file not availible
  200 - OK: file found and returned

To clean workspace:
  make clean
