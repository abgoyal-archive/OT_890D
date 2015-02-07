
#ifndef __MT6516_MM_QUEUE_H__
#define __MT6516_MM_QUEUE_H__

typedef enum 
{
    FLUSH_MM_QUEUE,
    PUSH_INFO_TO_MM_QUEUE,
    SHOW_TOP_INFO_FROM_MM_QUEUE,
    SHOW_MM_QUEUE,
    MAKE_PMEM_TO_NONCACHED,
    MAX_MM_QUEUE_CMD = 0xFFFFFFFF
} MM_QUEUE_CMD;

typedef struct mm_queue_info_t {
    unsigned char * yuv_buf_vaddr;           //va
    unsigned int yuv_buf_paddr;              //pa
    unsigned char * q_table_vaddr;           //va
    unsigned int q_table_paddr;              //pa
    unsigned char * int_sram_q_table_vaddr;  //va
    unsigned int int_sram_q_table_paddr;     //pa
} mm_queue_info;

typedef struct mm_queue_list_t {
    mm_queue_info info;
    struct mm_queue_list_t *next;
} mm_queue_list;

#endif //__MT6516_MM_QUEUE_H__
