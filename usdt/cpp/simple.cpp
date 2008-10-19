# include <stdio.h>
# include "simple_probes.h"

void fred(void);

int main(int argc, char **argv)
{
	/***********************************************/
	/*   Invoke shlib function.		       */
	/***********************************************/
	fred();

	while (1) {
		printf("here on line %d\n", __LINE__);
		SIMPLE_SAW_LINE(0x1234);
		printf("here on line %d\n", __LINE__);
		SIMPLE_SAW_WORD(0x87654321);
		printf("here on line %d\n", __LINE__);
		SIMPLE_SAW_WORD(0xdeadbeef);
		printf("here on line %d\n", __LINE__);
		sleep(1);
		}
}
