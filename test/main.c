#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //memset

#define __NR_ssel_pread64 316
#define __NR_ssel_pwrite64 317

int main()
{
	int devfd, randfd, imgfd, ret;
	char buf[20] = {0, };
	int buf_len = sizeof(buf);

	devfd = open("/dev/mydev", O_RDWR);
	if(devfd < 0)
	{
		printf("/dev/mydev open error : %d\n", errno);
		ret = -1;
		goto RETURN;
	}
	printf("/dev/mydev fd : %d\n", devfd);

	randfd = open("/dev/random", O_RDWR | O_NDELAY | O_CREAT, 0666);
	if(randfd < 0)
	{
		printf("test.txt open error\n");
		ret = -1;
		goto DEV_CLOSE;
	}
	printf("/dev/random fd : %d\n", randfd);

	imgfd = open("./test.txt", O_RDWR | O_NDELAY | O_CREAT, 0666);
	if(imgfd < 0)
	{
		printf("test.txt open error\n");
		ret = -1;
		goto RAND_CLOSE;
	}
	printf("test.txt fd : %d\n", imgfd);

	memset(buf, 0x00, buf_len);

	/*
	syscall(__NR_ssel_pread64, devfd, buf, buf_len, 0, randfd);
	printf("read data %s\n", buf);
	syscall(__NR_ssel_pwrite64, devfd, buf, buf_len, 0, imgfd);
	*/

	printf("count : %d\n", buf_len);
	syscall(__NR_ssel_pread64, devfd, (char*)buf, buf_len, 0, imgfd);
	//syscall(__NR_ssel_pread64, imgfd, buf, buf_len, 0, 0);

	sleep(5);

	printf("test.txt buf : %s\n", buf);
	//memcpy(buf,"sgfnsfgnsfgn",15);
	//syscall(__NR_ssel_pwrite64, imgfd, (const char*)buf, buf_len, 0, 0);
	//syscall(__NR_ssel_pwrite64, devfd, (const char*)buf, buf_len, 0, imgfd);
	

IMG_CLOSE:
	close(imgfd);
RAND_CLOSE:
	close(randfd);
DEV_CLOSE:
	close(devfd);
RETURN:
	return ret;
}
