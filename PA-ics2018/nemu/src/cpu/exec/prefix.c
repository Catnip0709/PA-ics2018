#include "cpu/exec.h"

make_EHelper(real);

make_EHelper(operand_size) {
  decoding.is_operand_size_16 = true;
  exec_real(eip);//exec_real返回的时候，eip会指向下一条指令
  decoding.is_operand_size_16 = false;

}
