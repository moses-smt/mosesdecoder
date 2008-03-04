This is the code to put moses on the web. It's buggy and a bit complicated to run.

1st compile the c++ executable. i use eclipse, the makefile doesn't work with this, it will create an exe called moses-cgi.

i've also included a linux precompiled version which u should use if u can. it may run on your system if u have the same library versions as me... (my computer is fedora 6) 


to run the system:
   1. make a subdirectory in the directory which can be seen by the apache server. cd into this directory
   2. make 2 named pipes called 'input' and 'output'
   3. copy moses-cgi into this directory, or softlink it into there 
   4. run the exe like
         ./moses-cgi -f moses.ini < input > output
   5. copy moses.php into the same directory, this is the html page people should be using to run the demo
   6. make sure that apache can execute moses.php and can access 'input' & 'output'
told u it's complicated !

if u want to see what it looks like, go to
    http://groups.inf.ed.ac.uk/hoang/demo/en-de/moses.php 
u're welcome to change the moses.php. please keep the link back to factored-translation.com

email me ((hieuhoang@gmail.com) if u have any queries, i'm sure u will...
   