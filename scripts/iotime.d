io:::start
{
	this->t = timestamp;
}
io:::done
/timestamp - this->t > 1000/
{
	printf("%-20s %d", execname, timestamp - this->t);
}
