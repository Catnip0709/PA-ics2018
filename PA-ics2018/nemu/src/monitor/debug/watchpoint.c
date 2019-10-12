#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#include <stdlib.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
//head用于组织使用中的监视点结构
//free用于组织空闲中的监视点结构


//对head和free和wp池初始化
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}


/* TODO: Implement the functionality of watchpoint */
extern uint32_t expr(char *e, bool *success);

WP* new_wp(char* args)
{
	if(free_==NULL)
	{
		assert(0);
		return NULL;
	}
	else
	{
		bool success=0;
		//从free_中取出一个监视点temp，并放到head中表示已被占用
		WP *temp=free_;
		free_=free_->next;

		if(head!=NULL)
			temp->next=head;
		else
			temp->next=NULL;
		head=&wp_pool[temp->NO];
		
		strcpy(head->expr,args);
		head->value=expr(args,&success);
		return temp;
	}
}


bool check_wp()
{
	bool result=0;
	WP* test=head;
	while(test!=NULL)
	{
		bool success=0;
		uint32_t temp_value=expr(test->expr, &success);
		if(temp_value!=test->value)
		{
			result=1;
			printf("meet watchpoint %d whoes expr = %s\n",test->NO,test->expr);
			//更新这个wp的value
			test->value=temp_value;
		}
		test=test->next;
	}
	return result;
}

void print_wp()
{
	WP *test=head;
	if(test==NULL)
		printf("without watchpoint\n");
	else
	{
		while(test!=NULL)
		{
			printf("NO=%d\texpr=%s\tvalue=%d\n",test->NO,test->expr,test->value);
			test=test->next;
		}
	}
}


void free_wp(char* args)
{
	WP *test=head;
	bool flag=0;
	int no = atoi(args);
	//查看第一个节点是不是目标节点
	if(test->NO == no)
	{
		head=head->next;
		test->next=free_;
		free_=test;
		printf("successfully delete watchpoint NO=%d\n", test->NO);
		return;
	}
	//第一个节点不是目标节点
	while(test->next!=NULL)
	{
		if(test->next->NO == no)
		{
			flag=1;
			break;
		}
		test=test->next;
	}
	if(flag==0)
	{
		printf("without this watchpoint, delete unsuccessfully\n");
		return ;
	}
	else
	{
		WP *temp=test->next;
		test->next=temp->next;
		temp->next=free_;
		free_=temp;
		printf("successfully delete watchpoint NO=%d\n", temp->NO);
	}
}