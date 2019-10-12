#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}
static int cmd_help(char *args);
static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
	if(args==NULL) //si
		cpu_exec(1);
	else//si N
	{
		int value=atoi(strtok(NULL, " "));
		cpu_exec(value);
	}
  return 0;
}
extern void print_wp();
static int cmd_info(char *args){
	if(strcmp(args,"r")==0) //info r
	{
	    for(int i=0;i<8;i++)
	    {
	       printf("%s\t",reg_name(i,4));
	       printf("0x%x\n",reg_l(i));
	    }
	    for(int i=0;i<8;i++)
	    {
	       printf("%s\t",reg_name(i,2));
	       printf("0x%x\n",reg_w(i));
	    }
	    for(int i=0;i<8;i++)
	    {
	       printf("%s\t",reg_name(i,1));
	       printf("0x%x\n",reg_b(i));
	    }		
	}

	else if(strcmp(args,"w")==0)//info w
	{
		print_wp();
	}

	return 0;
}

extern uint32_t expr(char *e, bool *success);
static int cmd_p(char *args){
	bool success;
	uint32_t result = expr(args, &success);
  printf("result=%d\n",result);
	return success;
}
static int cmd_x(char *args){
  char *len=strtok(args," ");
  char *addr=strtok(NULL," ");
  uint32_t length=0;
  sscanf(len,"%u",&length);

  bool success;
  uint32_t address = expr(addr,&success);

  for(int i=0;i<length;i++)
  {
    char copy_addr[20];   
    memset(&copy_addr,0,20);
    sprintf(copy_addr,"%x",address);
    printf("%s\t",copy_addr);
    printf("%x\n", vaddr_read(address,4)); 
    address+=4;
  }
  return 0;	
}
static int cmd_w(char *args){
  WP* wp = new_wp(args);
  printf("successfully got new point %d\n",wp->NO);
  return 0;  
}
extern void free_wp(char* args);
static int cmd_d(char *args){
  free_wp(args);
  return 0;
}
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Run 1 command of the program", cmd_si},
  { "info", "Print Info SUBCMD", cmd_info},
  { "p", "calculate the EXPR's value", cmd_p},
  { "x", "scan the memory", cmd_x},
  { "w", "set the watchpoint", cmd_w},
  { "d", "delete the watchpoint", cmd_d},
  /* TODO: Add more commands */
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}


//用户界面主循环
void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);
    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
