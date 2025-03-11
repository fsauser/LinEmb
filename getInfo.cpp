#include <iostream> 
#include <stdio.h>
#include <cstring>

using namespace std;

int main (int argc, char* argv[])
{
  int c, i, j, ncpu;
  char ip[64];
  char line[128];
  char cpu[32][128];
  
  FILE *f = popen("ip a | grep 'scope global' | awk '{print $2}' | cut -d '/' -f1", "r");
  
  c = getc(f);
  i = 0;
  while (c != EOF)
  {
    i += sprintf(ip+i, "%c", c);
    c = getc(f);
  }
  pclose(f);
  cout << "ip addr. : " << ip;
   
  f = fopen("/proc/cpuinfo", "r");
  i = 0;
  ncpu = 0;
  char str[]="model name";

  while (fgets(line, sizeof(line), f)) 
  {
    if (strncmp(line, str, sizeof(str)-1)==0) 
    {
      for(j=0;j<(int)sizeof(line)-1;j++)
      {
        if (line[j] == ':') break;
      }
      sprintf(&cpu[ncpu][0], "%s", &line[j+2]);
      ncpu++;   		
    }
  }
  fclose(f);
  cout << "Nbr of processor : " << ncpu << endl;
  for(i=0; i<ncpu;i++)
  {
    cout << "Processor " << i+1 << ": " << &cpu[i][0];
  }

  return 0;
}


