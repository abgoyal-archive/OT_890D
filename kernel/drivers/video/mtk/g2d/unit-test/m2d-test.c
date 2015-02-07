
#include <stdio.h>
#include <memory.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <time.h>


#include "../mtk_g2d.h"


static int test_bitblt(int m2d_drv)
{
    const unsigned int WIDTH = 4;
    const unsigned int HEIGHT = 4;
    const unsigned int BPP = 4;
    const unsigned int SIZE = WIDTH * HEIGHT * BPP;

    m2d_bitblt bitblt;

    void *srcBuffer = malloc(SIZE);
    void *dstBuffer = malloc(SIZE);
    unsigned int *ptr = (unsigned int *)srcBuffer;

    int i, cmp, ret = 0;

    memset(srcBuffer, 0, SIZE);
    memset(dstBuffer, 0, SIZE);

    srand(time(NULL));
    
    for (i = 0; i < SIZE / 4; ++ i) {
        ptr[i] = rand();
    }

    bitblt.srcSurf.virtAddr = (unsigned int)srcBuffer;
    bitblt.srcSurf.format = M2D_COLOR_ARGB8888;
    bitblt.srcSurf.pitchInPixels = WIDTH;
    bitblt.srcSurf.rect.x = bitblt.srcSurf.rect.y = 0;
    bitblt.srcSurf.rect.width = WIDTH;
    bitblt.srcSurf.rect.height = HEIGHT;

    bitblt.dstSurf.virtAddr = (unsigned int)dstBuffer;
    bitblt.dstSurf.format = M2D_COLOR_ARGB8888;
    bitblt.dstSurf.pitchInPixels = WIDTH;
    bitblt.dstSurf.rect.x = bitblt.dstSurf.rect.y = 0;
    bitblt.dstSurf.rect.width = WIDTH;
    bitblt.dstSurf.rect.height = HEIGHT;

    bitblt.enAlphaBlending = 0;
    bitblt.constAlphaValue = 0;

    bitblt.rotate = M2D_ROTATE_0;

    ret = ioctl(m2d_drv, M2D_IOC_BITBLT, &bitblt);

    if (ret)
    {
        printf("ERROR: call ioctl(M2D_IOC_BITBLT) failed, %d\n", ret);
        fflush(stdout);
    } else {
        if ((cmp = memcmp(srcBuffer, dstBuffer, SIZE)) != 0) {
            printf("ERROR: compare failed at %d bytes\n", cmp);
        } else {
            printf("MTK G2D Bitblt Unit Test Passed!\n");
        }
    }    
    
    free(srcBuffer);
    free(dstBuffer);

    return 0;
}


int main()
{
    int m2d_drv = 0;

    printf("test begin\n");
    fflush(stdout);
    
    m2d_drv = open("/dev/"M2D_DEV_NAME, O_RDWR, 0);

	if (-1 == m2d_drv)
	{
		printf("error: cannot open\n");
		fflush(stdout);
		return 0;
	}
	else
	{
		printf("open OK\n");
		fflush(stdout);
	}

    printf("test bitblt ...\n");

    test_bitblt(m2d_drv);

	close(m2d_drv);

	return 0; 
}

