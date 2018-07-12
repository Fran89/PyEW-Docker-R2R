
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

	SetEvent_ew(nh);

	fprintf(stdout, "event named 'test_event' sent\n");

	CloseHandle_ew(nh);
}
