#include <iostream>
#include <cstdlib>

#include <pthread.h>

using namespace std;

pthread_mutex_t cout_mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_data {
  short thread_id;
  string msg;
};

void *PrintHello(void *arg)
{
   thread_data* d = reinterpret_cast<thread_data*>(arg);
   cout << "Thread " << d->thread_id << " says: " << d->msg << endl;
   pthread_exit(NULL);
}

int main(int argc, char** argv)
{
  if (argc<2) {
    cout << "Usage: " << argv[0] << " nthreads" << endl;
    return 0;
  }

  const short nthreads = atoi(argv[1]);
  pthread_t threads[nthreads];
  thread_data data[nthreads];

  for(short i=0;i<nthreads;++i) {
    cout << "Creating thread " << i << endl;
    data[i].thread_id = i;
    data[i].msg = "Hello";
    int rc = pthread_create(&threads[i], NULL,  PrintHello,
                            reinterpret_cast<void*>(&data[i]) );
    if (rc) {
      cout << "Error:unable to create thread: " << rc << endl;
      return 1;
    }
  }
  for(short i=0;i<nthreads;++i) pthread_join(threads[i],NULL);

  //pthread_exit(NULL);

  return 0;
}
