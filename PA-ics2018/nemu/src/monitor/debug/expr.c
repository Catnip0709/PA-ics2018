#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <memory.h>

enum {
  TK_NOTYPE = 1, TK_EQ,TK_PLUS,TK_MINOR,
  TK_MUL,TK_DIV,
  TK_DEREF,
  TK_NUM,TK_HEXNUM,TK_LEFTBAR,TK_RIGHTBAR,
  TK_REGISTER,TK_ADDRESS
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces 1
  
  {"==", TK_EQ},        // equal== 2
  {"\\+", TK_PLUS},     // plus+  3
  {"\\-", TK_MINOR},    // 减号- 4

  {"\\*", TK_MUL},      // 乘号* 5
  {"\\/", TK_DIV},      // 除号/ 6

  {"\\*", TK_DEREF},    // 指针符号* 7
  
  {"0x([0-9a-f])+",TK_HEXNUM}, //十六进制数字 8
  {"([0-9])+",TK_NUM},         //数字 9
 
  {"\\(\\$([a-z])+\\)",TK_REGISTER}, //寄存器 10
  
  {"\\(",TK_LEFTBAR},          //左括号 11
  {"\\)",TK_RIGHTBAR}          //右括号 12  
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32]; //已识别出的token信息
int nr_token=0;   //已识别出的token数目

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;
  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        /*Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);*/
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        tokens[nr_token].type=rules[i].token_type;
        for(int i=0;i<substr_len;i++)  
          tokens[nr_token].str[i]=*(substr_start+i);
        nr_token++;

        break;
      }
    }
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}

//查看最外层是否是匹配的括号
uint32_t check_parentheses(int begin, int end) 
{
  //最左边是左括号，最右边是右括号
  if(tokens[begin].type==TK_LEFTBAR && tokens[end].type==TK_RIGHTBAR)
  {
    int climb=0;
    //用“上下坡”的概念实现判断是否合法
    for(int i=begin;i<end;i++)
    {
      if(tokens[i].type==TK_LEFTBAR)
        climb++;
      else if(tokens[i].type==TK_RIGHTBAR)
        climb--;
      if(i!=end-1 && climb==0)
        return false;
    }
    return true;
  }
  else
    return false;
}

uint32_t register_name_to_int(int i)
{        
  char name_reg[10];
  
  strcpy(name_reg,tokens[i].str+2);
  name_reg[strlen(name_reg)-1]='\0';

  int l=4,count=0;
  bool flag=0;

  while(true)
  {
    if(l<1)
      break;
    flag=strcmp(name_reg,reg_name(count,l));
    count++;
    if(count==8)
    {
      count=0;
      l=l/2;
    }
    if(!flag)
      break;
  } 

  if(flag)
  {
    printf("No such register\n");
    return 1;
  }
  else
  {
    if(l<1)
      l=1;
     char result[32];
     if(l==4)
       snprintf(result,32,"%d",reg_l(count));
     else if(l==2)
       snprintf(result,32,"%d",reg_w(count));
     else if(l==1)
       snprintf(result,32,"%d",reg_b(count));

     memset(tokens[i].str,0,sizeof(tokens[i]).str);
     strcpy(tokens[i].str,result);
     tokens[i].type=TK_NUM; 
  }  
  return 1;       
}

uint32_t evaluate(int begin,int end)
{
  if(begin>end)
    panic("bad expression");

  else if(begin==end)//single token should be a number, return it
  {
    if(tokens[begin].type==TK_HEXNUM) //16进制
    {
      uint32_t hex_num=0;
      sscanf(tokens[begin].str,"%x",&hex_num);
      return hex_num;
    }
    if(tokens[begin].type==TK_REGISTER) //寄存器
      {
        if(register_name_to_int(begin)==1)         
          return atoi(tokens[begin].str);
      }        
      
    else  //普通10进制
      return atoi(tokens[begin].str);
  }

  //括号匹配
  else if(check_parentheses(begin,end)==true)
    return evaluate(begin+1,end-1);

  else//找到优先级最低的符号
  {
    int position=0;
    bool flag=0;
    for(int i=begin;i<end;i++)
    {
      if(tokens[i].type==TK_LEFTBAR){
        flag=1;
        continue;
      }
      else if(tokens[i].type==TK_RIGHTBAR){
        flag=0;
        continue;
      }
      if(flag==0 && position==0 && (tokens[i].type==TK_PLUS  || 
                                    tokens[i].type==TK_MINOR ||
                                    tokens[i].type==TK_MUL   ||
                                    tokens[i].type==TK_DIV)     )
        position=i;
      else if (flag==0 && 
               (tokens[i].type==TK_MUL || tokens[i].type==TK_DIV) && 
               !(tokens[position].type==TK_PLUS || tokens[position].type==TK_MINOR))
        position=i;
      else if(flag==0 && (tokens[i].type==TK_PLUS || tokens[i].type==TK_MINOR))
        position=i;
    }
    uint32_t val1=evaluate(begin,position-1);
    uint32_t val2=evaluate(position+1,end);
    switch(tokens[position].type)
    {
      case TK_PLUS:  return val1+val2;
      case TK_MINOR: return val1-val2;
      case TK_MUL:   return val1*val2;
      case TK_DIV:   return val1/val2;
      default:assert(0);
    }
  }
  return -1;
}

uint32_t expr(char *e, bool *success) {
  /*(1)识别token*/
  if (!make_token(e)) {
    *success = false;
    printf("make_token wrong\n");
    return 0;
  }

  //遍历tokens，若遇到*判断是乘号还是指针符号
  for(int i=0;i<nr_token;i++)
    if(tokens[i].str[0]=='*' &&
       (i==0 || (tokens[i-1].type!=TK_NUM && tokens[i-1].type!=TK_RIGHTBAR)))
      tokens[i].type=TK_DEREF;

   /*（2）表达式求值*/
  uint32_t result = evaluate(0,nr_token-1);

  memset(&tokens,0,sizeof(Token)*32);
  
  return result;
}
