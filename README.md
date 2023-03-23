# CS344 Smallsh

<img align="right" width="300" height="300" src="https://user-images.githubusercontent.com/28117713/227071738-290bcdb5-4f17-4ba5-9e77-c39ca97393fb.png">

Smallsh is a simplified shell program that has the ability to manage background processes, print a prompt, read input, split input into words, perform expansion of certain tokens within words, and parse the words into tokens. The tokens will then be executed, and their output will be directed to an output file, an input file, or both. Smallsh handles signals and ignore any child state changes except for processes that have been stopped, which should be sent the SIGCONT signal. Additionally, Smallsh supports comments, which are lines starting with the "#" character, and the ability to run a command in the background by using the "&" character. Finally, Smallsh supports three PSx parameters, with only PS1 used in this implementation, and be able to expand tokens such as "~/" to the HOME environment variable, "$$" to the process ID of smallsh, "$?" to the exit status of the last foreground command, and "$!" to the process ID of the most recent background process.
