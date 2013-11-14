
#include "PreProcessFilter.h"

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <csignal>

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)

#include "Fdstream.h"

using namespace std;

#define CHILD_STDIN_READ pipefds_input[0]
#define CHILD_STDIN_WRITE pipefds_input[1]
#define CHILD_STDOUT_READ pipefds_output[0]
#define CHILD_STDOUT_WRITE pipefds_output[1]
#define CHILD_STDERR_READ pipefds_error[0]
#define CHILD_STDERR_WRITE pipefds_error[1]

namespace MosesTuning
{


// Child exec error signal
void exec_failed (int sig)
{
  cerr << "Exec failed. Child process couldn't be launched." << endl;
  exit (EXIT_FAILURE);
}

PreProcessFilter::PreProcessFilter(const string& filterCommand)
  : m_toFilter(NULL),
    m_fromFilter(NULL)
{
  // Child error signal install
  // sigaction is the replacement for the traditional signal() method
  struct sigaction action;
  action.sa_handler = exec_failed;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  if (sigaction(SIGUSR1, &action, NULL) < 0) {
    perror("SIGUSR1 install error");
    exit(EXIT_FAILURE);
  }

  int pipe_status;
  int pipefds_input[2];
  int pipefds_output[2];
  // int pipefds_error[2];

  // Create the pipes
  // We do this before the fork so both processes will know about
  // the same pipe and they can communicate.

  pipe_status = pipe(pipefds_input);
  if (pipe_status == -1) {
    perror("Error creating the pipe");
    exit(EXIT_FAILURE);
  }

  pipe_status = pipe(pipefds_output);
  if (pipe_status == -1) {
    perror("Error creating the pipe");
    exit(EXIT_FAILURE);
  }

  /*
  pipe_status = pipe(pipefds_error);
  if (pipe_status == -1)
  {
      perror("Error creating the pipe");
      exit(EXIT_FAILURE);
  }
  */

  pid_t pid;
  // Create child process; both processes continue from here
  pid = fork();

  if (pid == pid_t(0)) {
    // Child process

    // When the child process finishes sends a SIGCHLD signal
    // to the parent

    // Tie the standard input, output and error streams to the
    // appropiate pipe ends
    // The file descriptor 0 is the standard input
    // We tie it to the read end of the pipe as we will use
    // this end of the pipe to read from it
    dup2 (CHILD_STDIN_READ,0);
    dup2 (CHILD_STDOUT_WRITE,1);
    // dup2 (CHILD_STDERR_WRITE,2);
    // Close in the child the unused ends of the pipes
    close(CHILD_STDIN_WRITE);
    close(CHILD_STDOUT_READ);
    //close(CHILD_STDERR_READ);

    // Execute the program
    execl("/bin/bash", "bash", "-c", filterCommand.c_str() , (char*)NULL);

    // We should never reach this point
    // Tell the parent the exec failed
    kill(getppid(), SIGUSR1);
    exit(EXIT_FAILURE);
  } else if (pid > pid_t(0)) {
    // Parent

    // Close in the parent the unused ends of the pipes
    close(CHILD_STDIN_READ);
    close(CHILD_STDOUT_WRITE);
    // close(CHILD_STDERR_WRITE);

    m_toFilter = new ofdstream(CHILD_STDIN_WRITE);
    m_fromFilter = new ifdstream(CHILD_STDOUT_READ);
  } else {
    perror("Error: fork failed");
    exit(EXIT_FAILURE);
  }
}

string PreProcessFilter::ProcessSentence(const string& sentence)
{
  *m_toFilter << sentence << "\n";
  string processedSentence;
  m_fromFilter->getline(processedSentence);
  return processedSentence;
}

PreProcessFilter::~PreProcessFilter()
{
  delete m_toFilter;
  delete m_fromFilter;
}

}

#endif
