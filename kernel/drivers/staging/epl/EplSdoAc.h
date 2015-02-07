

#ifndef _EPLSDOAC_H_
#define _EPLSDOAC_H_

// =========================================================================
// SDO abort codes
// =========================================================================

#define    EPL_SDOAC_TIME_OUT                            0x05040000L
#define    EPL_SDOAC_UNKNOWN_COMMAND_SPECIFIER           0x05040001L
#define    EPL_SDOAC_INVALID_BLOCK_SIZE                  0x05040002L
#define    EPL_SDOAC_INVALID_SEQUENCE_NUMBER             0x05040003L
#define    EPL_SDOAC_OUT_OF_MEMORY                       0x05040005L
#define    EPL_SDOAC_UNSUPPORTED_ACCESS                  0x06010000L
#define    EPL_SDOAC_READ_TO_WRITE_ONLY_OBJ              0x06010001L
#define    EPL_SDOAC_WRITE_TO_READ_ONLY_OBJ              0x06010002L
#define    EPL_SDOAC_OBJECT_NOT_EXIST                    0x06020000L
#define    EPL_SDOAC_OBJECT_NOT_MAPPABLE                 0x06040041L
#define    EPL_SDOAC_PDO_LENGTH_EXCEEDED                 0x06040042L
#define    EPL_SDOAC_GEN_PARAM_INCOMPATIBILITY           0x06040043L
#define    EPL_SDOAC_INVALID_HEARTBEAT_DEC               0x06040044L
#define    EPL_SDOAC_GEN_INTERNAL_INCOMPATIBILITY        0x06040047L
#define    EPL_SDOAC_ACCESS_FAILED_DUE_HW_ERROR          0x06060000L
#define    EPL_SDOAC_DATA_TYPE_LENGTH_NOT_MATCH          0x06070010L
#define    EPL_SDOAC_DATA_TYPE_LENGTH_TOO_HIGH           0x06070012L
#define    EPL_SDOAC_DATA_TYPE_LENGTH_TOO_LOW            0x06070013L
#define    EPL_SDOAC_SUB_INDEX_NOT_EXIST                 0x06090011L
#define    EPL_SDOAC_VALUE_RANGE_EXCEEDED                0x06090030L
#define    EPL_SDOAC_VALUE_RANGE_TOO_HIGH                0x06090031L
#define    EPL_SDOAC_VALUE_RANGE_TOO_LOW                 0x06090032L
#define    EPL_SDOAC_MAX_VALUE_LESS_MIN_VALUE            0x06090036L
#define    EPL_SDOAC_GENERAL_ERROR                       0x08000000L
#define    EPL_SDOAC_DATA_NOT_TRANSF_OR_STORED           0x08000020L
#define    EPL_SDOAC_DATA_NOT_TRANSF_DUE_LOCAL_CONTROL   0x08000021L
#define    EPL_SDOAC_DATA_NOT_TRANSF_DUE_DEVICE_STATE    0x08000022L
#define    EPL_SDOAC_OBJECT_DICTIONARY_NOT_EXIST         0x08000023L
#define    EPL_SDOAC_CONFIG_DATA_EMPTY                   0x08000024L

#endif // _EPLSDOAC_H_

// Die letzte Zeile muﬂ unbedingt eine leere Zeile sein, weil manche Compiler
// damit ein Problem haben, wenn das nicht so ist (z.B. GNU oder Borland C++ Builder).
