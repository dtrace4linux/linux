#pragma D option quiet

io:::start
{
	start[args[0]->b_edev, args[0]->b_blkno] = timestamp;
}

io:::done
/start[args[0]->b_edev, args[0]->b_blkno]/
{
	/*
	 * We want to get an idea of our throughput to this device in KB/sec.
	 * What we have, however, is nanoseconds and bytes.  That is we want
	 * to calculate:
	 *
	 *                        bytes / 1024
	 *                  ------------------------
	 *                  nanoseconds / 1000000000
	 *
	 * But we can't calculate this using integer arithmetic without losing
	 * precision (the denomenator, for one, is between 0 and 1 for nearly
	 * all I/Os).  So we restate the fraction, and cancel:
	 * 
	 *     bytes      1000000000         bytes        976562
	 *   --------- * -------------  =  --------- * -------------  
	 *      1024      nanoseconds          1        nanoseconds
	 *
	 * This is easy to calculate using integer arithmetic; this is what
	 * we do below.
	 */
	this->elapsed = timestamp - start[args[0]->b_edev, args[0]->b_blkno];
	@[args[1]->dev_statname, args[1]->dev_pathname] =
	    quantize((args[0]->b_bcount * 976562) / this->elapsed);
	start[args[0]->b_edev, args[0]->b_blkno] = 0;
}

END
{
	printa("  %s (%s)\n%@d\n", @);
}
