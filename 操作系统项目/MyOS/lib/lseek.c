
#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

PUBLIC int lseek()
{

	MESSAGE msg;
	msg.type = LSEEK;

	
	send_recv(BOTH, TASK_FS, &msg);
	return msg.OFFSET;
}
