# UMUC Group Project
This project will be completed as part of the CMSC 495 - Current Trends and Projects in Computer Science course.

# Agent
The agent is intended to run as a daemon and execute tasks at scheduled intervals, as well as on-demand.

The agent is a normal Linux executable file (ELF format) and will run by being placed onto a computer and executed in any standard method.
Currently, the agent simply logs to /tmp/agent.log that it is alive and running.
There are no checks to ensure the agent is not already reading and the agent must be manually killed.

The simplest way to start the agent is to cd into the folder where the agent.exe file is located and to run the './agent.exe' command.
Down the road, I intend to create a Python configuration script that will allow the user (Antivirus Service Administrator) to configure the binary prior to pushing it to a remote machine. (Could be added to the management console.)

# Building the code
The code to build the Agent is located in the src folder. There is a Makefile in the agent directory that can be used to rebuild the code.

The command to compile from src is: 'make' which will defaultly run all targets that exist within the Makefile.
The executable is symbolically linked to the agent directory to allow for easier access and testing.

If you need to remove your executables to recompile, the command is: 'make clean'

# Agent Team
Seymour, Noah <jackjack178@gmail.com>

Turner, Daniel <dnlturner4@gmail.com>
