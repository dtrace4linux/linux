# if __KERNEL__

	# include <linux_types.h>
	# include <linux/zlib.h>
	# include "../port.h"

# else

	# include <zlib.h>
	# define	zlib_inflateInit inflateInit
	# define	zlib_inflate inflate
	# define	zlib_inflateEnd inflateEnd

# endif

static char *last_err;

int ctf_uncompress (char *dest, int *destLen, char *source, int sourceLen)
{
#if defined(DO_NOT_HAVE_ZLIB_IN_KERNEL)
	return Z_BUF_ERROR;
#else
	z_stream stream;
	int err;

	memset(&stream, 0, sizeof stream);

	stream.next_in = source;
	stream.avail_in = sourceLen;
	/* Check for source > 64K on 16-bit machine: */
	if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

	stream.next_out = dest;
	stream.avail_out = (uInt)*destLen;
	if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

	err = zlib_inflateInit(&stream);
	if (err != Z_OK) return err;

	err = zlib_inflate(&stream, Z_FINISH);
	last_err = stream.msg;
	if (err != Z_STREAM_END) {
		zlib_inflateEnd(&stream);
		if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
	        	return Z_DATA_ERROR;
		return err;
	}
	*destLen = stream.total_out;

	err = zlib_inflateEnd(&stream);
	return err;
#endif
}
char *
ctf_zstrerror(void)
{
	return last_err;
}

