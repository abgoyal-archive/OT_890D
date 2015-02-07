
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_CMD 28

typedef enum 
{
		PRESS_OK_KEY		= 1,
		RELEASE_OK_KEY		= 2,
		PRESS_MENU_KEY		= 3,
		RELEASE_MENU_KEY	= 4,
		PRESS_UP_KEY		= 5,
		RELEASE_UP_KEY		= 6,
		PRESS_DOWN_KEY		= 7,
		RELEASE_DOWN_KEY	= 8,
		PRESS_LEFT_KEY		= 9,
		RELEASE_LEFT_KEY	= 10,
		PRESS_RIGHT_KEY		= 11,
		RELEASE_RIGHT_KEY	= 12,
		PRESS_HOME_KEY		= 13,
		RELEASE_HOME_KEY	= 14,
		PRESS_BACK_KEY		= 15,
		RELEASE_BACK_KEY	= 16,
		PRESS_CALL_KEY		= 17,
		RELEASE_CALL_KEY	= 18,
		PRESS_HANG_KEY		= 19,
		RELEASE_HANG_KEY	= 20,
		PRESS_V_UP_KEY		= 21,
		RELEASE_V_UP_KEY	= 22,		
		PRESS_V_DOWN_KEY	= 23,
		RELEASE_V_DOWN_KEY	= 24,
		PRESS_FOCUS_KEY		= 25,
		RELEASE_FOCUS_KEY	= 26,
		PRESS_CAM_KEY		= 27,
		RELEASE_CAM_KEY		= 28        
} KPD_IOCTL_CMD;

char *cmd_str[] = 
{
    "OK_P\n", "OK_R\n", "MENU_P\n", "MENU_R\n", "UP_P\n", "UP_R\n", "DOWN_P\n", "DOWN_R\n",
    "LEFT_P\n", "LEFT_R\n", "RIGHT_P\n", "RIGHT_R\n", "HOME_P\n", "HOME_R\n", "BACK_P\n", "BACK_R\n",
    "CALL_P\n", "CALL_R\n", "HANG_P\n", "HANG_R\n", "V_UP_P\n", "V_UP_R\n", "V_DOWN_P\n", "V_DOWN_R\n",
    "FOCUS_P\n", "FOCUS_R\n", "CAM_P\n", "CAM_R\n"
};

int main(int argc, char *argv[])
{
    int kpd_fd = 0;
    FILE *pfd = NULL;
    char str[128];
    unsigned int round = 0;
    int match = 0;
    int i = 0;    
    int count = 1;
    int ioctl_num = 0;
    int forever = 0;
    clock_t time;
    unsigned int duration;
    
    if(argc < 2)
    {
        printf("Usage : kpd_auto <CONFIG_FILE_PATH> forever\n");
        return 0;
    }
        
    if(argc==3)
        forever = ((atoi(argv[2]) == 1)? 1 : 0);
    else if (argc==2)
        forever = 0;

    printf("forever = %d\n", forever);
    if((pfd=fopen(argv[1],"r"))==NULL) 
    {
        printf("Can't open file : %s\n", argv[1]);
        exit(1);
    }
    
    kpd_fd = open("/dev/mt6516-kpd", O_RDWR, 0);    
    if(kpd_fd == -1)
    {
        printf("----error: can't open mt6516-kpd----\n");
    }    
    
    do
    {
        rewind(pfd);  
        round++;
        time = clock();
        duration = time / CLOCKS_PER_SEC;
        printf("[KPD AUTO TEST] Already run %8u seconds\n", duration);
        printf("[KPD AUTO TEST] Round %8u ready GO !\n", round);    
        
        while(fgets(str, 128, pfd) != NULL)
        {
            count = 0;
            for(i=0; i < MAX_CMD; i++)
            {        
                if(! strcmp(str, cmd_str[i]))
                {            
                    ioctl_num = i + 1;
                    match = 1;
                    break;
                }
                if(str[0] == '#')
                {
                    //printf("# %c\n", str[1]);
                    count = atoi((str+1));
                    printf("delay = %d sec\n", count);
                    //count = (int)str[1] - 48;
                    break;
                }            
            }
            if(match)
            {
                //printf("count down time is %d\n", count);  
                //printf("ioctl_num = %d\n", ioctl_num);
                match = 0;
                ioctl(kpd_fd, ioctl_num, NULL);                       
            }
            sleep(count);
        }       
    }while(forever);
    fclose(pfd);
    close(kpd_fd);
    return 0;
}

