int me;			/* an integer global variable */
this int foo;		/* an integer clause-local variable */

tick-1sec
{
	/*
	 * Set foo to be 10 if and only if this is the first clause executed.
	 */
	this->foo = (me % 3 == 0) ? 10 : this->foo;
	printf("Clause 1 is number %d; foo is %d\n", me++ % 3, this->foo++);
}

tick-1sec
{
	/*
	 * Set foo to be 20 if and only if this is the first clause executed. 
	 */
	this->foo = (me % 3 == 0) ? 20 : this->foo;
	printf("Clause 2 is number %d; foo is %d\n", me++ % 3, this->foo++);
}

tick-1sec
{
	/*
	 * Set foo to be 30 if and only if this is the first clause executed.
	 */
	this->foo = (me % 3 == 0) ? 30 : this->foo;
	printf("Clause 3 is number %d; foo is %d\n", me++ % 3, this->foo++);
}
