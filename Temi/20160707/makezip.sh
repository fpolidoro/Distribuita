#!/bin/bash
mv socket.zip .socket.zip.old >& /dev/null 
zip -r socket.zip source/client/*.[ch] source/server[12]/*.[ch] source/*.[ch]
