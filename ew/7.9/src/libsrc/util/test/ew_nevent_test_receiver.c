
#include <stdio.h>
#include <stdlib.h>
#include <earthworm.h>
#include <ew_nevent_message.h>



main()
{

HANDLE nh;

char name[] = "test_event";


	logit_init("test", 0, 256, 1);

	nh = CreateEvent_ew(NULL, -1, 0, name);

	fprintf(stdout, "event named 'test_event' created\n");

	fprintf(stdout, "About to wait for event named 'test_event' sent\n");

	WaitForSingleObject_ew(nh, INFINITE);

	fprintf(stdout, "event named 'test_event' happened\n");

	CloseHandle_ew(nh);
}
