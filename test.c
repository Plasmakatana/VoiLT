#include<stdio.h>
#include "log.h"

int main(){
  log_init("testlog.txt");
  log_set_level(LOG_ERROR);
  log_message(LOG_ERROR,"%d %d",100,200);
  log_access("1.1.1.1", "GET", "/abc/xyz", 200, 500 );
  log_cleanup();
  return 0;
}
