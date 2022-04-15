#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "leveldb/db.h"
#include "Commands.h"
#include "WakeupThread.h"
#include "InputLine.h"

int main(int argc,const char ** argv )
{
	if ( argc < 2 )
	{
		printf("Usage: bitcoinstats <bitcoin-data-directory> (options)\n");
	}
	else
	{
		commands::Commands *c = commands::Commands::create(argc-1,argv+1);

		wakeupthread::WakeupThread *wt = wakeupthread::WakeupThread::create();
		inputline::InputLine *il = inputline::InputLine::create(wt);


		bool isExit = false;
		while ( !isExit )
		{
			wt->goToSleep(1000);
			const char *data = il->getInputLine();
			if ( data )
			{
				isExit = c->processInput(data);
			}
		}


		c->release();
		il->release();
		wt->release();
	}

	return 0;
}
