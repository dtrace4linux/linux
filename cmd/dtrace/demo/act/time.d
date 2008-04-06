#pragma D option quiet
#pragma D option destructive
#pragma D option switchrate=5sec

tick-1sec
/n++ < 5/
{
	printf("walltime  : %Y\n", walltimestamp);
	printf("date      : ");
	system("date");
	printf("\n");
}

tick-1sec
/n == 5/
{
	exit(0);
}
